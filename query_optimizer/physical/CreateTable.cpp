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

#include "query_optimizer/physical/CreateTable.hpp"

#include <string>
#include <vector>

#include "query_optimizer/OptimizerTree.hpp"
#include "query_optimizer/expressions/AttributeReference.hpp"
#include "utility/Cast.hpp"
#include "utility/Macros.hpp"

namespace quickstep {
namespace optimizer {
namespace physical {

void CreateTable::getFieldStringItems(
    std::vector<std::string> *inline_field_names,
    std::vector<std::string> *inline_field_values,
    std::vector<std::string> *non_container_child_field_names,
    std::vector<OptimizerTreeBaseNodePtr> *non_container_child_fields,
    std::vector<std::string> *container_child_field_names,
    std::vector<std::vector<OptimizerTreeBaseNodePtr>> *container_child_fields) const {
  inline_field_names->push_back("relation");
  inline_field_values->push_back(relation_name_);

  container_child_field_names->push_back("attributes");
  container_child_fields->push_back(CastSharedPtrVector<OptimizerTreeBase>(attributes_));

  // The tree representation of the LayoutDescription object, if specified by the user.
  if (block_properties_representation_) {
    non_container_child_field_names->push_back("block_properties");
    non_container_child_fields->push_back(block_properties_representation_);
  }

  if (partition_scheme_header_proto_representation_) {
    non_container_child_field_names->push_back("partition_scheme_header");
    non_container_child_fields->push_back(partition_scheme_header_proto_representation_);
  }
}

}  // namespace physical
}  // namespace optimizer
}  // namespace quickstep
