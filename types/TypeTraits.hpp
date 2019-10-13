/**
 *   Licensed to the Apache Software Foundation (ASF) under one
 *   or more contributor license agreements.  See the NOTICE file
 *   distributed with this work for additional information
 *   regarding copyright ownership.  The ASF licenses this file
 *   to you under the Apache License, Version 2.0 (the
 *   "License"); you may not use this file except in compliance
 *   with the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing,
 *   software distributed under the License is distributed on an
 *   "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *   KIND, either express or implied.  See the License for the
 *   specific language governing permissions and limitations
 *   under the License.
 *   limitations under the License.
 **/

#ifndef QUICKSTEP_TYPES_TYPE_TRAITS_HPP_
#define QUICKSTEP_TYPES_TYPE_TRAITS_HPP_

#include <cstdint>

#include "types/TypeID.hpp"

namespace quickstep {

class Type;

/** \addtogroup Types
 *  @{
 */

class IntType;
class LongType;
class FloatType;
class DoubleType;
class NullType;
template <std::int64_t> class DecimalType;

template <TypeID type_id>
struct TypeIDTrait;

#define REGISTER_TYPE(T, type_id, CppType) \
  template <> struct TypeIDTrait<type_id> { \
    typedef T TypeClass; \
    typedef CppType cpptype; \
    static constexpr TypeID kStaticTypeID = type_id; \
  }

REGISTER_TYPE(IntType, kInt, int);
REGISTER_TYPE(LongType, kLong, std::int64_t);
REGISTER_TYPE(FloatType, kFloat, float);
REGISTER_TYPE(DoubleType, kDouble, double);
REGISTER_TYPE(NullType, kNullType, void);

#undef REGISTER_TYPE

/** @} */

}  // namespace quickstep

#endif  // QUICKSTEP_TYPES_TYPE_TRAITS_HPP_
