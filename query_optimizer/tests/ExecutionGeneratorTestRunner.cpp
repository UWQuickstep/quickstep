/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 **/

#include "query_optimizer/tests/ExecutionGeneratorTestRunner.hpp"

#include <cstdio>
#include <fstream>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "catalog/CatalogDatabase.hpp"
#include "cli/CommandExecutor.hpp"
#include "cli/DropRelation.hpp"
#include "cli/PrintToScreen.hpp"
#include "parser/ParseStatement.hpp"
#include "query_execution/ForemanSingleNode.hpp"
#include "query_execution/QueryExecutionUtil.hpp"
#include "query_optimizer/LogicalGenerator.hpp"
#include "query_optimizer/Optimizer.hpp"
#include "query_optimizer/OptimizerContext.hpp"
#include "query_optimizer/PhysicalGenerator.hpp"
#include "query_optimizer/QueryHandle.hpp"
#include "query_optimizer/QueryProcessor.hpp"
#include "query_optimizer/logical/Logical.hpp"
#include "query_optimizer/physical/Physical.hpp"
#include "query_optimizer/tests/TestDatabaseLoader.hpp"
#include "storage/StorageConstants.hpp"
#include "storage/StorageManager.hpp"
#include "utility/MemStream.hpp"
#include "utility/SqlError.hpp"

#include "glog/logging.h"

namespace quickstep {

class CatalogRelation;

namespace optimizer {
namespace {

void GenerateAndPrintPhysicalPlan(const ParseStatement &parse_statement,
                                  CatalogDatabase *catalog_database,
                                  std::string *output) {
  OptimizerContext optimizer_context;
  LogicalGenerator logical_generator(&optimizer_context);
  PhysicalGenerator physical_generator(&optimizer_context);

  const logical::LogicalPtr logical_plan =
      logical_generator.generatePlan(*catalog_database, parse_statement);
  const physical::PhysicalPtr physical_plan =
      physical_generator.generatePlan(logical_plan, catalog_database);
  output->append(physical_plan->toString());
  output->append("--\n");
}

}  // namespace

const std::vector<std::string> ExecutionGeneratorTestRunner::kTestOptions = {
    "reset_before_execution", "print_physical_plan",
};

ExecutionGeneratorTestRunner::ExecutionGeneratorTestRunner(
    const std::string &storage_path)
    : storage_manager_(storage_path),
      thread_id_map_(ClientIDMap::Instance()) {
  // Create the default catalog file.
  const std::string catalog_path = storage_path + kCatalogFilename;
  std::ofstream catalog_file(catalog_path);
  CHECK(catalog_file.good())
      << "ERROR: Unable to open " << catalog_path << " for writing.";

  Catalog catalog;
  catalog.addDatabase(new CatalogDatabase(nullptr, "default"));
  CHECK(catalog.getProto().SerializeToOstream(&catalog_file))
      << "ERROR: Unable to serialize catalog proto to file " << storage_path;
  catalog_file.close();

  // Create query processor and initialize the test database.
  query_processor_ = std::make_unique<QueryProcessor>(std::string(catalog_path));

  test_database_loader_ = std::make_unique<TestDatabaseLoader>(
      query_processor_->getDefaultDatabase(), &storage_manager_),
  test_database_loader_->createTestRelation(false /* allow_vchar */);
  test_database_loader_->loadTestRelation();

  bus_.Initialize();

  main_thread_client_id_ = bus_.Connect();
  bus_.RegisterClientAsSender(main_thread_client_id_, kAdmitRequestMessage);
  bus_.RegisterClientAsSender(main_thread_client_id_, kPoisonMessage);
  bus_.RegisterClientAsReceiver(main_thread_client_id_, kWorkloadCompletionMessage);

  worker_.reset(new Worker(0, &bus_));

  std::vector<client_id> worker_client_ids;
  worker_client_ids.push_back(worker_->getBusClientID());

  // We don't use the NUMA aware version of foreman code.
  std::vector<int> numa_nodes;
  numa_nodes.push_back(-1);

  workers_.reset(new WorkerDirectory(1 /* number of workers */,
                                     worker_client_ids, numa_nodes));
  foreman_.reset(
      new ForemanSingleNode(main_thread_client_id_,
                            workers_.get(),
                            &bus_,
                            query_processor_->getDefaultDatabase(),
                            &storage_manager_));

  foreman_->start();
  worker_->start();
}

void ExecutionGeneratorTestRunner::runTestCase(
    const std::string &input, const std::set<std::string> &options,
    std::string *output) {
  // TODO(qzeng): Test multi-threaded query execution when we have a Sort operator.

  VLOG(4) << "Test SQL(s): " << input;

  if (options.find(kTestOptions[0]) != options.end()) {
    test_database_loader_->clear();
    test_database_loader_->createTestRelation(false /* allow_vchar */);
    test_database_loader_->loadTestRelation();
  }

  MemStream output_stream;
  sql_parser_.feedNextBuffer(new std::string(input));

  // Redirect stderr to output_stream.
  if (!FLAGS_logtostderr) {
    stderr = output_stream.file();
  }

  while (true) {
    ParseResult result = sql_parser_.getNextStatement();
    if (result.condition != ParseResult::kSuccess) {
      if (result.condition == ParseResult::kError) {
        output->append(result.error_message);
      }
      break;
    }

    const ParseStatement &statement = *result.parsed_statement;
    if (statement.getStatementType() == ParseStatement::kCommand) {
      try {
        cli::executeCommand(statement,
                            *(query_processor_->getDefaultDatabase()),
                            main_thread_client_id_,
                            foreman_->getBusClientID(),
                            &bus_,
                            &storage_manager_,
                            query_processor_.get(),
                            output_stream.file());
      } catch (const quickstep::SqlError &sql_error) {
        fprintf(stderr, "%s", sql_error.formatMessage(input).c_str());
      }
      output->append(output_stream.str());
      continue;
    }

    const CatalogRelation *query_result_relation = nullptr;
    try {
      if (options.find(kTestOptions[1]) != options.end()) {
        GenerateAndPrintPhysicalPlan(statement,
                                     query_processor_->getDefaultDatabase(),
                                     output);
      }
      auto query_handle = std::make_unique<QueryHandle>(
          query_processor_->query_id(), main_thread_client_id_);

      query_processor_->generateQueryHandle(statement, query_handle.get());
      DCHECK(query_handle->getQueryPlanMutable() != nullptr);

      query_result_relation = query_handle->getQueryResultRelation();

      QueryExecutionUtil::ConstructAndSendAdmitRequestMessage(
          main_thread_client_id_, foreman_->getBusClientID(),
          query_handle.release(), &bus_);
    } catch (const SqlError &error) {
      output->append(error.formatMessage(input));
      break;
    }

    QueryExecutionUtil::ReceiveQueryCompletionMessage(main_thread_client_id_,
                                                      &bus_);

    if (query_result_relation) {
      PrintToScreen::PrintRelation(*query_result_relation,
                                   &storage_manager_,
                                   output_stream.file());
      DropRelation::Drop(*query_result_relation,
                         test_database_loader_->catalog_database(),
                         test_database_loader_->storage_manager());
    }
    output->append(output_stream.str());
  }
}

}  // namespace optimizer
}  // namespace quickstep
