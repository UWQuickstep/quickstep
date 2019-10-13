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

#ifndef QUICKSTEP_UTILITY_META_TYPE_LIST_HPP_
#define QUICKSTEP_UTILITY_META_TYPE_LIST_HPP_

#include <cstddef>
#include <type_traits>

#include "utility/meta/Common.hpp"
#include "utility/meta/TypeListMetaFunctions.hpp"

namespace quickstep {
namespace meta {

/** \addtogroup Utility
 *  @{
 */

/**
 * @brief A compile-time list of C++ types.
 **/
template <typename ...Ts> class TypeList;

namespace internal {

using EmptyList = TypeList<>;

template <typename ...Ts>
class TypeListBase {
 public:
  // ---------------------------------------------------------------------------
  // Members

  static constexpr std::size_t length = sizeof...(Ts);

  using type = TypeList<Ts...>;
  using self = type;

  // ---------------------------------------------------------------------------
  // Meta methods

  /**
   * @brief Bind the elements of this TypeList to the specified template.
   *
   * Example:
   * --
   * using Input = TypeList<int, float>;
   * using Output = Input::bind_to<std::tuple>
   * --
   * -> Output == std::tuple<int, float>
   */
  template <template <typename ...> class Host>
  using bind_to = Host<Ts...>;

  /**
   * @brief Get the element at the specified index(es). Each index starts at 0.
   *
   * Example:
   * --
   * using Input = TypeList<int, TypeList<float, double>>;
   * using Output1 = Input::at<0>;  // first element
   * using Output2 = Input::at<1, 1>;  // second element's second element
   * --
   * -> Output1 == int
   *    Output2 == double
   */
  template <std::size_t ...pos>
  using at = typename ElementAt<
      self, TypeList<std::integral_constant<std::size_t, pos>...>>::type;

  /**
   * @brief Get a subset TypeList of the first n elements.
   *
   * Example:
   * --
   * using Input = TypeList<int, float, double>;
   * using Output = Input::take<2>;
   * --
   * -> Output == TypeList<int, float>
   */
  template <std::size_t n>
  using take = typename Take<self, EmptyList, n>::type;

  /**
   * @brief Get a subset TypeList by removing the first n elements.
   *
   * Example:
   * --
   * using Input = TypeList<int, float, double>;
   * using Output = Input::skip<1>;
   * --
   * -> Output == TypeList<float, double>
   */
  template <std::size_t n>
  using skip = typename Skip<self, n>::type;

  /**
   * @brief Get a new TypeList by prepending the specified element to the
   *        beginning of this TypeList.
   *
   * Example:
   * --
   * using Input = TypeList<int, float>;
   * using Output = Input::push_front<double>;
   * --
   * -> Output == TypeList<double, int, float>
   */
  template <typename T>
  using push_front = TypeList<T, Ts...>;

  /**
   * @brief Get a new TypeList by appending the specified element to the end of
   *        this TypeList.
   *
   * Example:
   * --
   * using Input = TypeList<int, float>;
   * using Output = Input::push_back<double>;
   * --
   * -> Output == TypeList<int, float, double>
   */
  template <typename T>
  using push_back = TypeList<Ts..., T>;

  /**
   * @brief Check if this TypeList contains the specified element.
   *
   * Example:
   * --
   * using Input = TypeList<int, float>;
   * using Output1 = Input::contains<float>;
   * using Output2 = Input::contains<double>;
   * --
   * -> Output1::value == true
   *    Output2::value == false
   */
  template <typename T>
  using contains = EqualsAny<T, Ts...>;

  /**
   * @brief Get the unique elements in this TypeList.
   *
   * Example:
   * --
   * using Input = TypeList<int, float, int, double>;
   * using Output = Input::unique<>;
   * --
   * -> Output == TypeList<int, float, double>
   */
  template <typename ...DumbT>
  using unique = typename Unique<EmptyList, self, DumbT...>::type;

  /**
   * @brief Get a new TypeList by concatenating the specified TypeList to the
   *        end of this TypeList.
   *
   * Example:
   * --
   * using Input1 = TypeList<int, float>;
   * using Input2 = TypeList<long, double>;
   * using Output = Input1::append<Input2>;
   * --
   * -> Output == TypeList<int, float, long, double>
   */
  template <typename TL>
  using append = typename Append<self, TL>::type;

  /**
   * @brief Get a new TypeList that is the cartesian product of this TypeList
   *        and the specified TypeList.
   *
   * Example:
   * --
   * using Input1 = TypeList<int, float>;
   * using Input2 = TypeList<long, double>;
   * using Output = Input1::cartesian_product<Input2>;
   * --
   * -> Output == TypeList<TypeList<int, long>,
   *                       TypeList<int, double>,
   *                       TypeList<float, long>,
   *                       TypeList<float, double>>
   */
  template <typename TL>
  using cartesian_product = typename CartesianProduct<self, TL>::type;

  /**
   * @brief Get the set difference between this TypeList and the specified
   *        TypeList.
   *
   * Example:
   * --
   * using Input1 = TypeList<int, float, int, double>;
   * using Input2 = TypeList<bool, int, double>;
   * using Output = Input1::subtract<Input2>;
   * --
   * -> Output == TypeList<float>
   */
  template <typename Subtrahend>
  using subtract = typename Subtract<EmptyList, self, Subtrahend>::type;

  /**
   * @brief Get a new TypeList by applying the specified Op to all the elements
   *        in this TypeList.
   *
   * Example:
   * --
   * template <typename T>
   * struct GetSize {
   *   using type = std::integral_constant<std::size_t, sizeof(T)>;
   * };
   *
   * using Input = TypeList<int, float, double>;
   * using Output = Input::apply<GetSize>;
   * --
   * -> Output == TypeList<std::integral_constant<std::size_t, 4>,
   *                       std::integral_constant<std::size_t, 4>,
   *                       std::integral_constant<std::size_t, 8>>
   */
  template <template <typename ...> class Op>
  using apply = TypeList<typename Op<Ts>::type...>;

  /**
   * @brief Get a new TypeList by applying the specified Op to all the elements
   *        in this TypeList and concatenating the results.
   *
   * Example:
   * --
   * template <typename T>
   * struct Duplicate {
   *   using type = std::TypeList<T, T>;
   * };
   *
   * using Input = TypeList<int, float, double>;
   * using Output = Input::flatmap<Duplicate>;
   * --
   * -> Output == TypeList<int, int, float, float, double, double>
   */
  template <template <typename ...> class Op>
  using flatmap = typename Flatmap<EmptyList, self, Op>::type;

  /**
   * @brief Get a new TypeList by filtering the elements with the specified
   *        predicate.
   *
   * Example:
   * --
   * template <typename T>
   * struct HasSizeFour {
   *   static constexpr bool value = (sizeof(T) == 4);
   * };
   *
   * using Input = TypeList<int, float, double>;
   * using Output = Input::filter<HasSizeFour>;
   * --
   * -> Output == TypeList<int, float>
   */
  template <template <typename ...> class Op>
  using filter = typename Filter<EmptyList, self, Op>::type;

  /**
   * @brief Get a new TypeList by applying the specified Op to all the elements
   *        in this TypeList. For each element T, Op<T>::type is in the result
   *        if and only if it is well-formed.
   *
   * Example:
   * --
   * template <typename T>
   * struct TypeListToTuple;
   *
   * template <typename ...Ts>
   * struct TypeListToTuple<TypeList<Ts...>> {
   *   using type = std::tuple<Ts...>;
   * };
   *
   * using Input = TypeList<int, TypeList<float, float>, double>;
   * using Output = Input::filtermap<TypeListToTuple>;
   * --
   * -> Output == TypeList<std::tuple<float, float>>
   */
  template <template <typename ...> class Op>
  using filtermap = typename Filtermap<EmptyList, self, Op>::type;


  /**
   * @brief Get a new TypeList by flattening all nested TypeLists in the elements.
   *
   * Example:
   * --
   * using Input = TypeList<int, TypeList<bool, TypeList<float, int>>, double>;
   * using Output = Input::flatten<>;
   * --
   * -> Output == TypeList<int, bool, float, int, double>
   */
  template <typename ...DumbT>
  using flatten = typename Flatten<EmptyList, self, DumbT...>::type;

  /**
   * @brief Get a new TypeList by flattening only one level of nested TypeLists
   *        in the elements.
   *
   * Example:
   * --
   * using Input = TypeList<int, TypeList<bool, TypeList<float, int>>, double>;
   * using Output = Input::flatten_once<>;
   * --
   * -> Output == TypeList<int, bool, TypeList<float, int>, double>
   */
  template <typename ...DumbT>
  using flatten_once = typename FlattenOnce<EmptyList, self, DumbT...>::type;

  /**
   * @brief Fold all the elements from left to right with the specified combiner
   *        Op and initial value Init.
   *
   * Example:
   * --
   * template <typename T>
   * struct GetSize {
   *   using type = std::integral_constant<std::size_t, sizeof(T)>;
   * };
   *
   * template <typename Lhs, typename Rhs>
   * struct Sum {
   *   using type = std::integral_constant<std::size_t, Lhs::value + Rhs::value>;
   * };
   *
   * using ZeroValue = std::integral_constant<std::size_t, 0>;
   *
   * // Calculate the total size of all the types in Input.
   * using Input = TypeList<int, float, double>;
   * using Output = Input::apply<GetSize>::foldl<Sum, ZeroValue>;
   * --
   * -> Output::value == 16
   */
  template <template <typename ...> class Op, typename Init>
  using foldl = typename Foldl<Init, self, Op>::type;

  /**
   * @brief Get a new TypeList by splitting this TypeList into length-n pieces.
   *        The last piece will be shorter if n does not evenly divide the
   *        length of the list.
   *
   * Example:
   * --
   * using Input = TypeList<int, long, float, double>;
   * using Output = Input::chunk_of<2>;
   * --
   * -> Output == TypeList<TypeList<int, long>, TypeList<float, double>>
   */
  template <std::size_t n>
  using chunks_of = typename ChunksOf<EmptyList, EmptyList, self, n, n>::type;

  /**
   * @brief Get a new TypeList by zipping values from this TypeList and the
   *        specified TypeList.
   *
   * Example:
   * --
   * using Input1 = TypeList<int, double>;
   * using Input2 = TypeList<long, float>;
   * using Output = Input1::zip<Input2>;
   * --
   * -> Output == TypeList<TypeList<int, long>, TypeList<double, float>>
   */
  template <typename TL>
  using zip = typename Zip<EmptyList, self, TL>::type;

  /**
   * @brief Get a new TypeList by applying the specified Op to each pair of
   *        zipped values from this TypeList and the specified TypeList.
   *
   * Example:
   * --
   * template <typename Lhs, typename Rhs>
   * struct MakePair {
   *   using type = std::pair<Lhs, Rhs>;
   * };
   *
   * using Input1 = TypeList<int, double>;
   * using Input2 = TypeList<long, float>;
   * using Output = Input1::zip_with<Input2, MakePair>;
   * --
   * -> Output == TypeList<std::pair<int, long>, std::pair<double, float>>
   */
  template <typename TL, template <typename ...> class Op>
  using zip_with = typename ZipWith<EmptyList, self, TL, Op>::type;

  /**
   * @brief Cast this TypeList to a Sequence. Require that each element in this
   *        TypeList has a "value" field.
   *
   * Example:
   * --
   * template <typename T>
   * struct GetSize {
   *   using type = std::integral_constant<std::size_t, sizeof(T)>;
   * };
   *
   * using Input = TypeList<int, float, double>;
   * using Output = Input::apply<GetSize>::as_sequence<std::size_t>;
   * --
   * -> Output == Sequence<std::size_t, 4, 4, 8>;
   */
  template <typename T>
  using as_sequence = typename AsSequence<T, Ts...>::type;

  // ---------------------------------------------------------------------------
  // Static methods

  /**
   * @brief Invoke the lambda function for each element in this TypeList. Each
   *        element T will be forwarded as an instance of TypeList<T> to the
   *        lambda function.
   *
   * Example:
   * --
   * using Input = TypeList<int, float, double>;
   *
   * Input::ForEach([](auto e) {
   *   using T = typename decltype(e)::head;
   *   std::cout << "Size = " << sizeof(T) << "\n";
   * });
   * --
   * -> Console output:
   * Size = 4
   * Size = 4
   * Size = 8
   */
  template <typename Functor>
  static inline void ForEach(const Functor &functor) {
    ForEachInternal<length == 0>(functor);
  }

 private:
  template <bool empty, typename Functor>
  static inline void ForEachInternal(const Functor &functor,
                                     std::enable_if_t<empty> * = 0) {
    // No-op
  }

  template <bool empty, typename Functor>
  static inline void ForEachInternal(const Functor &functor,
                                     std::enable_if_t<!empty> * = 0) {
    functor(take<1>());
    skip<1>::ForEach(functor);
  }
};

template <typename ...Ts>
constexpr std::size_t TypeListBase<Ts...>::length;

}  // namespace internal

template <typename T, typename ...Ts>
class TypeList<T, Ts...> : public internal::TypeListBase<T, Ts...> {
 public:
  using head = T;
  using tail = TypeList<Ts...>;
};

template <>
class TypeList<> : public internal::TypeListBase<> {};

/** @} */

}  // namespace meta
}  // namespace quickstep

#endif  // QUICKSTEP_UTILITY_META_TYPE_LIST_HPP_
