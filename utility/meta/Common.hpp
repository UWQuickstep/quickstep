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

#ifndef QUICKSTEP_UTILITY_META_COMMON_HPP_
#define QUICKSTEP_UTILITY_META_COMMON_HPP_

#include <cstddef>
#include <type_traits>

namespace quickstep {
namespace meta {

/** \addtogroup Utility
 *  @{
 */

/**
 * @brief Calculate the logical conjunction of the specified arguments.
 *        Each argument B must define member "value" that is convertible to bool.
 */
template <typename ...> struct Conjunction : std::true_type {};
template <typename B> struct Conjunction<B> : B {};
template <typename B, typename ...Bs>
struct Conjunction<B, Bs...>
    : std::conditional_t<B::value, Conjunction<Bs...>, B> {};

/**
 * @brief Calculate the logical disjunction of the specified arguments.
 *        Each argument B must define member "value" that is convertible to bool.
 */
template <typename ...> struct Disjunction : std::false_type {};
template <typename B> struct Disjunction<B> : B {};
template <typename B, typename ...Bs>
struct Disjunction<B, Bs...>
    : std::conditional_t<B::value, B, Disjunction<Bs...>> {};

/**
 * @brief Check if the first argument equals any of the later arguments.
 */
template <typename check, typename ...cases>
struct EqualsAny : Disjunction<std::is_same<check, cases>...> {};

template <typename T, T check, T ...cases>
struct EqualsAnyValue {
  static constexpr bool value =
     Disjunction<std::is_same<std::integral_constant<T, check>,
                              std::integral_constant<T, cases>>...>::value;
};

/**
 * @brief A compile-time sequence of constant values.
 */
template <typename T, T ...s>
struct Sequence {
  template <template <typename ...> class Host>
  using bind_to = Host<std::integral_constant<T, s>...>;

  template <template <T ...> class Host>
  using bind_values_to = Host<s...>;

  template <typename U>
  using cast_to = Sequence<U, static_cast<U>(s)...>;

  template <T v>
  using contains = EqualsAny<std::integral_constant<T, v>,
                             std::integral_constant<T, s>...>;

  template <typename Collection>
  inline static Collection Instantiate() {
    return { s... };
  }
};

/**
 * @brief A compile-time sequence of std::size_t values.
 */
template <std::size_t ...s>
using IntegerSequence = Sequence<std::size_t, s...>;

namespace internal {

template <std::size_t n, std::size_t ...s>
struct MakeSequence_0 : MakeSequence_0<n-1, n-1, s...> {};

template <std::size_t ...s>
struct MakeSequence_0<0, s...> {
  using type = IntegerSequence<s...>;
};

}  // namespace internal

/**
 * @brief Generate a compile-time sequence of std::size_t values.
 */
template <std::size_t n>
using MakeSequence = typename internal::MakeSequence_0<n>::type;

/**
 * @brief Checks if a type has member type with name "type".
 */
template <typename T, typename Enable = void>
struct IsCanonicalTrait {
  static constexpr bool value = false;
};

template <typename T>
struct IsCanonicalTrait<T, std::enable_if_t<
    std::is_same<typename T::type, typename T::type>::value>> {
  static constexpr bool value = true;
};

/**
 * @brief Checks whether Op<T> is well-formed.
 */
template <typename T, template <typename> class Op, typename Enable = void>
struct IsWellFormed {
  static constexpr bool value = false;
};

template <typename T, template <typename> class Op>
struct IsWellFormed<T, Op, std::enable_if_t<std::is_same<Op<T>, Op<T>>::value>> {
  static constexpr bool value = true;
};

namespace internal {

template <typename T, std::size_t = sizeof(T)>
std::true_type IsCompleteTypeImpl(T *);

std::false_type IsCompleteTypeImpl(...);

}  // namespace internal

/**
 * @brief Checks whether Op<T> is a complete type.
 */
template <typename T>
using IsCompleteType = decltype(internal::IsCompleteTypeImpl(std::declval<T*>()));

/**
 * @brief Trait wrapper.
 */
template <template <typename ...> class Op>
struct TraitWrapper {
 private:
  template <typename ...ArgTypes>
  struct Trait {
    using type = Op<ArgTypes...>;
  };

 public:
  template <typename ...ArgTypes>
  using type = Trait<ArgTypes...>;
};

/**
 * @brief Add two compile-time constants.
 */
template <typename Lhs, typename Rhs>
struct MetaAdd {
  using type = std::integral_constant<
      typename Lhs::value_type, Lhs::value + Rhs::value>;
};

// -----------------------------------------------------------------------------
// Macros.

#define QUICKSTEP_TRAIT_HAS_STATIC_METHOD(traitname, methodname) \
  template <typename C> \
  class traitname { \
   private: \
    template <typename T> \
    static std::true_type Impl(int, decltype(T::methodname) * = 0); \
    template <typename T> \
    static std::false_type Impl(...); \
   public: \
    static constexpr bool value = decltype(Impl<C>(0))::value; \
  }

/** @} */

}  // namespace meta
}  // namespace quickstep

#endif  // QUICKSTEP_UTILITY_META_COMMON_HPP_
