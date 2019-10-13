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

#ifndef QUICKSTEP_UTILITY_META_TYPE_LIST_META_FUNCTIONS_HPP_
#define QUICKSTEP_UTILITY_META_TYPE_LIST_META_FUNCTIONS_HPP_

#include <cstddef>
#include <type_traits>

#include "utility/meta/Common.hpp"

namespace quickstep {
namespace meta {

/** \addtogroup Utility
 *  @{
 */

// Forward declaration of TypeList.
template <typename ...Ts> class TypeList;

/**
 * @brief Type trait that checks whether T is an instance of TypeList.
 */
template <typename T>
struct IsTypeList {
  constexpr static bool value = false;
};
template <typename ...Ts>
struct IsTypeList<TypeList<Ts...>> {
  constexpr static bool value = true;
};

namespace internal {

/**
 * @brief TypeList meta function ElementAt.
 */
template <typename TL, typename PosTL, typename Enable = void>
struct ElementAt;

template <typename TL, typename PosTL>
struct ElementAt<TL, PosTL, std::enable_if_t<PosTL::length == 0>> {
  using type = TL;
};

template <typename TL, typename PosTL>
struct ElementAt<TL, PosTL, std::enable_if_t<PosTL::length != 0>>
    : ElementAt<typename std::tuple_element<
                    PosTL::head::value,
                    typename TL::template bind_to<std::tuple>>::type,
                typename PosTL::tail> {};


/**
 * @brief TypeList meta function Append.
 */
template <typename TL, typename TailTL>
struct Append;

template <typename ...Ts, typename ...Tails>
struct Append<TypeList<Ts...>, TypeList<Tails...>> {
  using type = TypeList<Ts..., Tails...>;
};


/**
 * @brief TypeList meta function Take.
 */
template <typename TL, typename Out, std::size_t rest, typename Enable = void>
struct Take;

template <typename TL, typename Out, std::size_t rest>
struct Take<TL, Out, rest, std::enable_if_t<rest == 0>> {
  using type = Out;
};

template <typename TL, typename Out, std::size_t rest>
struct Take<TL, Out, rest, std::enable_if_t<rest != 0>>
    : Take<typename TL::tail,
           typename Out::template push_back<typename TL::head>,
           rest - 1> {};


/**
 * @brief TypeList meta function Skip.
 */
template <typename TL, std::size_t rest, typename Enable = void>
struct Skip;

template <typename TL, std::size_t rest>
struct Skip<TL, rest, std::enable_if_t<rest == 0>> {
  using type = TL;
};

template <typename TL, std::size_t rest>
struct Skip<TL, rest, std::enable_if_t<rest != 0>>
    : Skip<typename TL::tail, rest - 1> {};


/**
 * @brief TypeList meta function Skip.
 */
template <typename Out, typename Rest, typename Enable = void>
struct Unique;

template <typename Out, typename Rest>
struct Unique<Out, Rest, std::enable_if_t<Rest::length == 0>> {
  using type = Out;
};

template <typename Out, typename Rest>
struct Unique<Out, Rest,
              std::enable_if_t<Out::template contains<typename Rest::head>::value>>
    : Unique<Out, typename Rest::tail> {};

template <typename Out, typename Rest>
struct Unique<Out, Rest,
              std::enable_if_t<!Out::template contains<typename Rest::head>::value>>
    : Unique<typename Out::template push_back<typename Rest::head>,
             typename Rest::tail> {};


/**
 * @brief TypeList meta function Subtract.
 */
template <typename Out, typename Rest, typename Subtrahend, typename Enable = void>
struct Subtract;

template <typename Out, typename Rest, typename Subtrahend>
struct Subtract<Out, Rest, Subtrahend, std::enable_if_t<Rest::length == 0>> {
  using type = Out;
};

template <typename Out, typename Rest, typename Subtrahend>
struct Subtract<Out, Rest, Subtrahend,
                std::enable_if_t<
                    Subtrahend::template contains<typename Rest::head>::value>>
    : Subtract<Out, typename Rest::tail, Subtrahend> {};

template <typename Out, typename Rest, typename Subtrahend>
struct Subtract<Out, Rest, Subtrahend,
                std::enable_if_t<
                    !Subtrahend::template contains<typename Rest::head>::value>>
    : Subtract<typename Out::template push_back<typename Rest::head>,
               typename Rest::tail, Subtrahend> {};


/**
 * @brief TypeList meta function CartesianProduct.
 */
template <typename LeftTL, typename RightTL>
struct CartesianProduct {
  template <typename LeftT>
  struct LeftHelper {
    template <typename RightT>
    struct RightHelper {
      using type = TypeList<LeftT, RightT>;
    };
    using type = typename RightTL::template apply<RightHelper>;
  };
  using type = typename LeftTL::template flatmap<LeftHelper>;
};


/**
 * @brief TypeList meta function Flatmap.
 */
template <typename Out, typename Rest, template <typename ...> class Op,
          typename Enable = void>
struct Flatmap;

template <typename Out, typename Rest, template <typename ...> class Op>
struct Flatmap<Out, Rest, Op, std::enable_if_t<Rest::length == 0>> {
  using type = Out;
};

template <typename Out, typename Rest, template <typename ...> class Op>
struct Flatmap<Out, Rest, Op, std::enable_if_t<Rest::length != 0>>
    : Flatmap<typename Out::template append<typename Op<typename Rest::head>::type>,
              typename Rest::tail, Op> {};


/**
 * @brief TypeList meta function Filter.
 */
template <typename Out, typename Rest, template <typename ...> class Op,
          typename Enable = void>
struct Filter;

template <typename Out, typename Rest, template <typename ...> class Op>
struct Filter<Out, Rest, Op, std::enable_if_t<Rest::length == 0>> {
  using type = Out;
};

template <typename Out, typename Rest, template <typename ...> class Op>
struct Filter<Out, Rest, Op, std::enable_if_t<Op<typename Rest::head>::value>>
    : Filter<typename Out::template push_back<typename Rest::head>,
             typename Rest::tail, Op> {};

template <typename Out, typename Rest, template <typename ...> class Op>
struct Filter<Out, Rest, Op, std::enable_if_t<!Op<typename Rest::head>::value>>
    : Filter<Out, typename Rest::tail, Op> {};


/**
 * @brief TypeList meta function Filtermap.
 */
template <typename Out, typename Rest, template <typename ...> class Op,
          typename Enable = void>
struct Filtermap;

template <typename Out, typename Rest, template <typename ...> class Op>
struct Filtermap<Out, Rest, Op, std::enable_if_t<Rest::length == 0>> {
  using type = Out;
};

template <typename Out, typename Rest, template <typename ...> class Op>
struct Filtermap<Out, Rest, Op,
                 std::enable_if_t<Rest::length != 0 &&
                                  IsCanonicalTrait<Op<typename Rest::head>>::value>>
    : Filtermap<typename Out::template push_back<typename Op<typename Rest::head>::type>,
                typename Rest::tail, Op> {};

template <typename Out, typename Rest, template <typename ...> class Op>
struct Filtermap<Out, Rest, Op,
                 std::enable_if_t<Rest::length != 0 &&
                                  !IsCanonicalTrait<Op<typename Rest::head>>::value>>
    : Filtermap<Out, typename Rest::tail, Op> {};


/**
 * @brief TypeList meta function Flatten.
 */
template <typename Out, typename Rest, typename Enable = void>
struct Flatten;

template <typename Out, typename Rest>
struct Flatten<Out, Rest, std::enable_if_t<Rest::length == 0>> {
  using type = Out;
};

template <typename Out, typename Rest>
struct Flatten<Out, Rest,
               std::enable_if_t<Rest::length != 0 &&
                                IsTypeList<typename Rest::head>::value>>
    : Flatten<typename Out::template append<typename Rest::head::template flatten<>>,
              typename Rest::tail> {};

template <typename Out, typename Rest>
struct Flatten<Out, Rest,
               std::enable_if_t<Rest::length != 0 &&
                                !IsTypeList<typename Rest::head>::value>>
    : Flatten<typename Out::template push_back<typename Rest::head>,
              typename Rest::tail> {};


/**
 * @brief TypeList meta function FlattenOnce.
 */
template <typename Out, typename Rest, typename Enable = void>
struct FlattenOnce;

template <typename Out, typename Rest>
struct FlattenOnce<Out, Rest, std::enable_if_t<Rest::length == 0>> {
  using type = Out;
};

template <typename Out, typename Rest>
struct FlattenOnce<Out, Rest, std::enable_if_t<Rest::length != 0>>
    : FlattenOnce<typename Out::template append<typename Rest::head>,
                  typename Rest::tail> {};


/**
 * @brief TypeList meta function Foldl.
 */
template <typename Out, typename Rest, template <typename ...> class Op,
          typename Enable = void>
struct Foldl;

template <typename Out, typename Rest, template <typename ...> class Op>
struct Foldl<Out, Rest, Op, std::enable_if_t<Rest::length == 0>> {
  using type = Out;
};

template <typename Out, typename Rest, template <typename ...> class Op>
struct Foldl<Out, Rest, Op, std::enable_if_t<Rest::length != 0>>
    : Foldl<typename Op<Out, typename Rest::head>::type,
            typename Rest::tail, Op> {};


/**
 * @brief TypeList meta function ChunksOf.
 */
template <typename Out, typename Accumulate, typename Rest,
          std::size_t n, std::size_t k, typename Enable = void>
struct ChunksOf {
  static_assert(n == 0, "Stride width must be a positive integer.");
};

template <typename Out, typename Accumulate, typename Rest,
          std::size_t n, std::size_t k>
struct ChunksOf<Out, Accumulate, Rest, n, k,
                std::enable_if_t<n != 0 && Rest::length == 0 &&
                                 Accumulate::length == 0>> {
  using type = Out;
};

template <typename Out, typename Accumulate, typename Rest,
          std::size_t n, std::size_t k>
struct ChunksOf<Out, Accumulate, Rest, n, k,
                std::enable_if_t<n != 0 && Rest::length == 0 &&
                                 Accumulate::length != 0>> {
  using type = typename Out::template push_back<Accumulate>;
};

template <typename Out, typename Accumulate, typename Rest,
          std::size_t n, std::size_t k>
struct ChunksOf<Out, Accumulate, Rest, n, k,
                  std::enable_if_t<n != 0 && Rest::length != 0 && k == 0>>
    : ChunksOf<typename Out::template push_back<Accumulate>,
               TypeList<typename Rest::head>,
               typename Rest::tail, n, n-1> {};

template <typename Out, typename Accumulate, typename Rest,
          std::size_t n, std::size_t k>
struct ChunksOf<Out, Accumulate, Rest, n, k,
                  std::enable_if_t<n != 0 && Rest::length != 0 && k != 0>>
    : ChunksOf<Out,
               typename Accumulate::template push_back<typename Rest::head>,
               typename Rest::tail, n, k-1> {};

/**
 * @brief TypeList meta function Zip.
 */
template <typename Out, typename RestL, typename RestR, typename Enable = void>
struct Zip;

template <typename Out, typename RestL, typename RestR>
struct Zip<Out, RestL, RestR,
           std::enable_if_t<RestL::length == 0 || RestR::length == 0>> {
  static_assert(RestL::length == 0 && RestR::length == 0,
                "Zip failed: TypeLists have unequal lengths");
  using type = Out;
};

template <typename Out, typename RestL, typename RestR>
struct Zip<Out, RestL, RestR,
           std::enable_if_t<RestL::length != 0 && RestR::length != 0>>
    : Zip<typename Out::template push_back<
              TypeList<typename RestL::head, typename RestR::head>>,
          typename RestL::tail, typename RestR::tail> {};


/**
 * @brief TypeList meta function ZipWith.
 */
template <typename Out, typename RestL, typename RestR,
          template <typename ...> class Op, typename Enable = void>
struct ZipWith;

template <typename Out, typename RestL, typename RestR,
          template <typename ...> class Op>
struct ZipWith<Out, RestL, RestR, Op,
               std::enable_if_t<RestL::length == 0 || RestR::length == 0>> {
  static_assert(RestL::length == 0 && RestR::length == 0,
                "ZipWith failed: TypeLists have unequal lengths");
  using type = Out;
};

template <typename Out, typename RestL, typename RestR,
          template <typename ...> class Op>
struct ZipWith<Out, RestL, RestR, Op,
               std::enable_if_t<RestL::length != 0 && RestR::length != 0>>
    : ZipWith<typename Out::template push_back<
                  typename Op<typename RestL::head, typename RestR::head>::type>,
              typename RestL::tail, typename RestR::tail, Op> {};


/**
 * @brief TypeList meta function AsSequence.
 */
template <typename T, typename ...Ts>
struct AsSequence {
  using type = Sequence<T, static_cast<T>(Ts::value)...>;
};

}  // namespace internal

/** @} */

}  // namespace meta
}  // namespace quickstep

#endif  // QUICKSTEP_UTILITY_META_TYPE_LIST_META_FUNCTIONS_HPP_
