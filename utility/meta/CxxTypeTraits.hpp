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

#ifndef QUICKSTEP_UTILITY_META_CXX_TYPE_TRAITS_HPP_
#define QUICKSTEP_UTILITY_META_CXX_TYPE_TRAITS_HPP_

#include <cstddef>
#include <cstdint>

#include "utility/meta/Common.hpp"

namespace quickstep {
namespace meta {

/** \addtogroup Utility
 *  @{
 */

/**
 * @brief C++ supported integer types.
 */
using CxxSupportedIntegerSizes = meta::IntegerSequence<1u, 2u, 4u, 8u>;

/**
 * @brief Get C++ unsigned integer type from size.
 */
template <std::size_t size> struct UnsignedInteger;

template <> struct UnsignedInteger<1u> {
  using type = std::uint8_t;
};
template <> struct UnsignedInteger<2u> {
  using type = std::uint16_t;
};
template <> struct UnsignedInteger<4u> {
  using type = std::uint32_t;
};
template <> struct UnsignedInteger<8u> {
  using type = std::uint64_t;
};

/** @} */

}  // namespace meta
}  // namespace quickstep

#endif  // QUICKSTEP_UTILITY_META_CXX_TYPE_TRAITS_HPP_
