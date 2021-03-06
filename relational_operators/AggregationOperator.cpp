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

#include "relational_operators/AggregationOperator.hpp"

#include <vector>

#include "catalog/CatalogTypedefs.hpp"
#include "query_execution/QueryContext.hpp"
#include "query_execution/WorkOrderProtosContainer.hpp"
#include "query_execution/WorkOrdersContainer.hpp"
#include "relational_operators/WorkOrder.pb.h"
#include "storage/AggregationOperationState.hpp"
#include "storage/StorageBlockInfo.hpp"
#include "utility/lip_filter/LIPFilterAdaptiveProber.hpp"
#include "utility/lip_filter/LIPFilterUtil.hpp"

#include "tmb/id_typedefs.h"

namespace quickstep {

bool AggregationOperator::getAllWorkOrders(
    WorkOrdersContainer *container,
    QueryContext *query_context,
    StorageManager *storage_manager,
    const tmb::client_id scheduler_client_id,
    tmb::MessageBus *bus) {
  if (input_relation_is_stored_) {
    if (started_) {
      return true;
    }

    for (partition_id part_id = 0; part_id < num_partitions_; ++part_id) {
      for (const block_id input_block_id : input_relation_block_ids_[part_id]) {
        container->addNormalWorkOrder(
            new AggregationWorkOrder(
                query_id_,
                part_id,
                input_block_id,
                query_context->getAggregationState(aggr_state_index_, part_id),
                CreateLIPFilterAdaptiveProberHelper(lip_deployment_index_, query_context)),
            op_index_);
      }
    }
    started_ = true;
    return true;
  } else {
    for (partition_id part_id = 0; part_id < num_partitions_; ++part_id) {
      while (num_workorders_generated_[part_id] < input_relation_block_ids_[part_id].size()) {
        container->addNormalWorkOrder(
            new AggregationWorkOrder(
                query_id_,
                part_id,
                input_relation_block_ids_[part_id][num_workorders_generated_[part_id]],
                query_context->getAggregationState(aggr_state_index_, part_id),
                CreateLIPFilterAdaptiveProberHelper(lip_deployment_index_, query_context)),
            op_index_);
        ++num_workorders_generated_[part_id];
      }
    }
    return done_feeding_input_relation_;
  }
}

bool AggregationOperator::getAllWorkOrderProtos(WorkOrderProtosContainer *container) {
  if (input_relation_is_stored_) {
    if (started_) {
      return true;
    }

    for (partition_id part_id = 0; part_id < num_partitions_; ++part_id) {
      for (const block_id input_block_id : input_relation_block_ids_[part_id]) {
        container->addWorkOrderProto(createWorkOrderProto(input_block_id, part_id), op_index_);
      }
    }
    started_ = true;
    return true;
  } else {
    for (partition_id part_id = 0; part_id < num_partitions_; ++part_id) {
      while (num_workorders_generated_[part_id] < input_relation_block_ids_[part_id].size()) {
        container->addWorkOrderProto(
            createWorkOrderProto(input_relation_block_ids_[part_id][num_workorders_generated_[part_id]], part_id),
            op_index_);
        ++num_workorders_generated_[part_id];
      }
    }
    return done_feeding_input_relation_;
  }
}

serialization::WorkOrder* AggregationOperator::createWorkOrderProto(const block_id block, const partition_id part_id) {
  serialization::WorkOrder *proto = new serialization::WorkOrder;
  proto->set_work_order_type(serialization::AGGREGATION);
  proto->set_query_id(query_id_);

  proto->SetExtension(serialization::AggregationWorkOrder::block_id, block);
  proto->SetExtension(serialization::AggregationWorkOrder::aggr_state_index, aggr_state_index_);
  proto->SetExtension(serialization::AggregationWorkOrder::partition_id, part_id);
  proto->SetExtension(serialization::AggregationWorkOrder::lip_deployment_index, lip_deployment_index_);

  for (const QueryContext::lip_filter_id lip_filter_index : lip_filter_indexes_) {
    proto->AddExtension(serialization::AggregationWorkOrder::lip_filter_indexes, lip_filter_index);
  }

  return proto;
}

void AggregationWorkOrder::execute() {
  state_->aggregateBlock(input_block_id_, lip_filter_adaptive_prober_.get());
}

}  // namespace quickstep
