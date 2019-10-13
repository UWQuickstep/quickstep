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

#ifndef QUICKSTEP_UTILITY_META_STRING_CONSTANT_HPP_
#define QUICKSTEP_UTILITY_META_STRING_CONSTANT_HPP_

#include <string>

#include "utility/meta/Common.hpp"
#include "utility/meta/TypeList.hpp"

namespace quickstep {
namespace meta {

/** \addtogroup Utility
 *  @{
 */

/**
 * @brief A compile-time string constant.
 */
template <char ...characters>
class StringConstant {
 private:
  // Type trait that checks whether CharValue is an integral constant that does
  // not equal 0.
  template <typename CharValue>
  struct IsNotNull {
    static constexpr bool value = (CharValue::value != 0);
  };

  // Remove trailing zeros of the \p characters sequence.
  using SanitizedCharacters = typename meta::Sequence<char, characters...>
                                           ::template bind_to<meta::TypeList>
                                           ::template filter<IsNotNull>
                                           ::template as_sequence<char>;

 public:
  /**
   * @brief Create a run-time string from the compile-time \p characters sequence.
   */
  static std::string ToString() {
    return SanitizedCharacters::template Instantiate<std::string>();
  }
};

// -----------------------------------------------------------------------------
// Macros.

#define STR_CONST_GET(str, index) \
  (index < sizeof(str) ? str[index] : 0)

#define STR_CONST_UNFOLD_4(str, addr) \
  STR_CONST_GET(str, (addr + 0x0)), STR_CONST_GET(str, (addr + 0x1)), \
  STR_CONST_GET(str, (addr + 0x2)), STR_CONST_GET(str, (addr + 0x3))

#define STR_CONST_UNFOLD_8(str, addr) \
  STR_CONST_UNFOLD_4(str, (addr + 0x0)), STR_CONST_UNFOLD_4(str, (addr + 0x4))

#define STR_CONST_UNFOLD_16(str, addr) \
  STR_CONST_UNFOLD_8(str, (addr + 0x0)), STR_CONST_UNFOLD_8(str, (addr + 0x8))

#define STR_CONST_UNFOLD_64(str, addr) \
  STR_CONST_UNFOLD_16(str, (addr + 0x00)), STR_CONST_UNFOLD_16(str, (addr + 0x10)), \
  STR_CONST_UNFOLD_16(str, (addr + 0x20)), STR_CONST_UNFOLD_16(str, (addr + 0x30))

/**
 * @brief Create a StringConstant from a C string literal that has up to 8
 *        characters.
 */
#define STR_CONST8(str) \
  meta::StringConstant<STR_CONST_UNFOLD_8(str, 0)>

/**
 * @brief Create a StringConstant from a C string literal that has up to 16
 *        characters.
 */
#define STR_CONST16(str) \
  meta::StringConstant<STR_CONST_UNFOLD_16(str, 0)>

/**
 * @brief Create a StringConstant from a C string literal that has up to 64
 *        characters.
 */
#define STR_CONST64(str) \
  meta::StringConstant<STR_CONST_UNFOLD_64(str, 0)>

/** @} */

}  // namespace meta
}  // namespace quickstep

#endif  // QUICKSTEP_UTILITY_META_STRING_CONSTANT_HPP_
