# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

# Declare micro-libs:
add_library(quickstep_utility_meta_Common ../../empty_src.cpp Common.hpp)
add_library(quickstep_utility_meta_CxxTypeTraits ../../empty_src.cpp CxxTypeTraits.hpp)
add_library(quickstep_utility_meta_Meta ../../empty_src.cpp Meta.hpp)
add_library(quickstep_utility_meta_MultipleDispatcher ../../empty_src.cpp MultipleDispatcher.hpp)
add_library(quickstep_utility_meta_StringConstant ../../empty_src.cpp StringConstant.hpp)
add_library(quickstep_utility_meta_TypeList ../../empty_src.cpp TypeList.hpp)
add_library(quickstep_utility_meta_TypeListMetaFunctions ../../empty_src.cpp TypeListMetaFunctions.hpp)

# Link dependencies:
target_link_libraries(quickstep_utility_meta_CxxTypeTraits
                      quickstep_utility_meta_Common)
target_link_libraries(quickstep_utility_meta_Meta
                      quickstep_utility_meta_Common
                      quickstep_utility_meta_CxxTypeTraits
                      quickstep_utility_meta_MultipleDispatcher
                      quickstep_utility_meta_StringConstant
                      quickstep_utility_meta_TypeList)
target_link_libraries(quickstep_utility_meta_MultipleDispatcher
                      glog
                      quickstep_utility_meta_Common
                      quickstep_utility_meta_TypeList)
target_link_libraries(quickstep_utility_meta_StringConstant
                      quickstep_utility_meta_Common
                      quickstep_utility_meta_TypeList)
target_link_libraries(quickstep_utility_meta_TypeList
                      quickstep_utility_meta_Common
                      quickstep_utility_meta_TypeListMetaFunctions)
target_link_libraries(quickstep_utility_meta_TypeListMetaFunctions
                      quickstep_utility_meta_Common)
