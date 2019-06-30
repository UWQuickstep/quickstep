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

#ifndef QUICKSTEP_QUERY_OPTIMIZER_TESTS_EXECUTION_GENERATOR_TEST_RUNNER_HPP_
#define QUICKSTEP_QUERY_OPTIMIZER_TESTS_EXECUTION_GENERATOR_TEST_RUNNER_HPP_

#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "parser/SqlParserWrapper.hpp"
#include "query_execution/ForemanSingleNode.hpp"
#include "query_execution/QueryExecutionTypedefs.hpp"
#include "query_execution/Worker.hpp"
#include "query_execution/WorkerDirectory.hpp"
#include "query_execution/WorkerMessage.hpp"
#include "query_optimizer/QueryProcessor.hpp"
#include "query_optimizer/tests/TestDatabaseLoader.hpp"
#include "storage/StorageManager.hpp"
#include "threading/ThreadIDBasedMap.hpp"
#include "utility/Macros.hpp"
#include "utility/textbased_test/TextBasedTestDriver.hpp"

#include "tmb/id_typedefs.h"
#include "tmb/message_bus.h"

namespace quickstep {
namespace optimizer {

/**
 * @brief TextBasedTestRunner for testing the ExecutionGenerator.
 */
class ExecutionGeneratorTestRunner : public TextBasedTestRunner {
 public:
  /**
   * List of test options.
   * --
   * reset_before_execution: Recreate the entire database and repopulate the
   *                         data before every test.
   * print_physical_plan: Also print the optimized physical plan.
   */
  static const std::vector<std::string> kTestOptions;

  /**
   * @brief Constructor.
   */
  explicit ExecutionGeneratorTestRunner(const std::string &storage_path);

  ~ExecutionGeneratorTestRunner() {
    QueryExecutionUtil::BroadcastPoisonMessage(main_thread_client_id_, &bus_);
    worker_->join();
    foreman_->join();
  }

  void runTestCase(const std::string &input,
                   const std::set<std::string> &options,
                   std::string *output) override;

 private:
  SqlParserWrapper sql_parser_;
  StorageManager storage_manager_;
  std::unique_ptr<QueryProcessor> query_processor_;
  std::unique_ptr<TestDatabaseLoader> test_database_loader_;

  MessageBusImpl bus_;
  std::unique_ptr<ForemanSingleNode> foreman_;
  std::unique_ptr<Worker> worker_;

  std::unique_ptr<WorkerDirectory> workers_;

  tmb::client_id main_thread_client_id_;

  // This map is needed for InsertDestination and some operators that send
  // messages to Foreman directly. To know the reason behind the design of this
  // map, see the note in InsertDestination.hpp.
  ClientIDMap *thread_id_map_;

  DISALLOW_COPY_AND_ASSIGN(ExecutionGeneratorTestRunner);
};

}  // namespace optimizer
}  // namespace quickstep

#endif /* QUICKSTEP_QUERY_OPTIMIZER_TESTS_EXECUTION_GENERATOR_TEST_RUNNER_HPP_ */
