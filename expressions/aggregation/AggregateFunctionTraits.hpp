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

#ifndef QUICKSTEP_PROTOTYPE_AGGREGATE_FUNCTION_TRAITS_HPP_
#define QUICKSTEP_PROTOTYPE_AGGREGATE_FUNCTION_TRAITS_HPP_

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <limits>
#include <type_traits>

#include "expressions/aggregation/AggregationID.hpp"
#include "types/TypeID.hpp"
#include "types/TypeTraits.hpp"
#include "utility/meta/Common.hpp"
#include "utility/meta/TypeList.hpp"

#include "glog/logging.h"

namespace quickstep {

/** \addtogroup Expressions
 *  @{
 */

namespace internal {

template <bool> struct HasZeroInitTrueType : std::true_type{};
template <typename T> HasZeroInitTrueType<T::kZeroInit> HasZeroInitCheck(int);
template <typename> std::false_type HasZeroInitCheck(...);

template <typename T>
struct HasZeroInit : decltype(HasZeroInitCheck<T>(0)) {};

template <typename T, typename Enable = void>
struct IsZeroInit;

template <typename T>
struct IsZeroInit<T, std::enable_if_t<!HasZeroInit<T>::value>> {
  static constexpr bool value = false;
};
template <typename T>
struct IsZeroInit<T, std::enable_if_t<HasZeroInit<T>::value>> {
  static constexpr bool value = T::kZeroInit;
};

}  // namespace internal


template <AggregationID agg_id, TypeID value_tid, typename Enable = void>
struct AggregationContext;

template <AggregationID agg_id, TypeID value_tid>
struct AggregationUnsafeOps;

template <AggregationID agg_id, TypeID value_tid>
struct AggregationAtomicOps;

template <AggregationID agg_id, TypeID value_tid, bool atomic>
class AggregateFunctionTrait;

template <AggregationID agg_id, TypeID value_tid>
class AggregateFunctionTrait<agg_id, value_tid, true> {
 private:
  using Context = AggregationContext<agg_id, value_tid>;
  using Op = AggregationAtomicOps<agg_id, value_tid>;

 public:
  using ValueType = typename TypeIDTrait<value_tid>::cpptype;
  using StateType = typename Context::AtomicStateT;

  static constexpr TypeID kResultTypeID =
      AggregationContext<agg_id, value_tid>::kResultTypeID;
  using ResultType = typename TypeIDTrait<kResultTypeID>::cpptype;

  static constexpr bool kZeroInit = internal::IsZeroInit<Op>::value;

  static inline void InitState(StateType *state) {
    Op::InitStateAtomic(state);
  }

  static inline void MergeValue(StateType *state, const ValueType &value) {
    Op::MergeValueAtomic(state, value);
  }

  static inline void MergeState(StateType *dst, const StateType &src) {
    Op::MergeStateAtomic(dst, src);
  }

  static inline void FinalizeState(ResultType *result, const StateType &state) {
    Op::FinalizeAtomic(result, state);
  }
};

template <AggregationID agg_id, TypeID value_tid>
class AggregateFunctionTrait<agg_id, value_tid, false> {
 private:
  using Context = AggregationContext<agg_id, value_tid>;
  using Op = AggregationUnsafeOps<agg_id, value_tid>;

 public:
  using ValueType = typename TypeIDTrait<value_tid>::cpptype;
  using StateType = typename Context::StateT;

  static constexpr TypeID kResultTypeID =
      AggregationContext<agg_id, value_tid>::kResultTypeID;
  using ResultType = typename TypeIDTrait<kResultTypeID>::cpptype;

  static constexpr bool kZeroInit = internal::IsZeroInit<Op>::value;

  static inline void InitState(StateType *state) {
    Op::InitStateUnsafe(state);
  }

  static inline void MergeValue(StateType *state, const ValueType &value) {
    Op::MergeValueUnsafe(state, value);
  }

  static inline void MergeState(StateType *dst, const StateType &src) {
    Op::MergeStateUnsafe(dst, src);
  }

  static inline void FinalizeState(ResultType *result, const StateType &state) {
    Op::FinalizeUnsafe(result, state);
  }
};


// Utilities
// -----------------------------------------------------------------------------
template <bool atomic>
struct AggregateFunctionTransformer {
  template <typename TL>
  struct transform {
    static constexpr bool kSuccess = true;
    using type = meta::TypeList<
        AggregateFunctionTrait<TL::template at<0>::value,
                               TL::template at<1>::value,
                               atomic>>;
  };
};


// States
// -----------------------------------------------------------------------------
template <TypeID value_tid>
struct AggregationContext<kCount, value_tid> {
  using StateT = std::int64_t;
  using AtomicStateT = std::atomic<StateT>;
  static constexpr TypeID kResultTypeID = kLong;
};

template <TypeID value_tid>
struct AggregationContext<
    kSum, value_tid, std::enable_if_t<
        meta::EqualsAnyValue<TypeID, value_tid, kInt, kLong>::value>> {
  using StateT = std::int64_t;
  using AtomicStateT = std::atomic<std::int64_t>;
  static constexpr TypeID kResultTypeID = kLong;
};

template <TypeID value_tid>
struct AggregationContext<
    kSum, value_tid, std::enable_if_t<
        meta::EqualsAnyValue<TypeID, value_tid, kFloat, kDouble>::value>> {
  using StateT = double;
  using AtomicStateT = std::atomic<StateT>;
  static constexpr TypeID kResultTypeID = kDouble;
};

template <TypeID value_tid>
struct AggregationContext<kAvg, value_tid> {
  using SumCtx = AggregationContext<kSum, value_tid>;
  using StateT = std::pair<typename SumCtx::StateT, std::int64_t>;
  using AtomicStateT = std::pair<typename SumCtx::AtomicStateT,
                                 std::atomic<std::int64_t>>;
  static constexpr TypeID kResultTypeID = value_tid;
};

template <TypeID value_tid>
struct AggregationContext<kMin, value_tid> {
  using StateT = struct { typename TypeIDTrait<value_tid>::cpptype value;
                          bool valid; };
  using AtomicStateT = std::atomic<StateT>;
  static constexpr TypeID kResultTypeID = value_tid;
};

template <TypeID value_tid>
struct AggregationContext<kMax, value_tid> {
  using StateT = struct { typename TypeIDTrait<value_tid>::cpptype value;
                          bool valid; };
  using AtomicStateT = std::atomic<StateT>;
  static constexpr TypeID kResultTypeID = value_tid;
};

template <TypeID value_tid>
struct AggregationContext<kHasMultipleValues, value_tid> {
  using StateT = struct { typename TypeIDTrait<value_tid>::cpptype value;
                          char status; };
  using AtomicStateT = std::atomic<StateT>;
  static constexpr TypeID kResultTypeID = kInt;
};

// Unsafe operators
// -----------------------------------------------------------------------------
template <TypeID value_tid>
struct AggregationUnsafeOps<kCount, value_tid> {
  using ValueT = typename TypeIDTrait<value_tid>::cpptype;
  using StateT = typename AggregationContext<kSum, value_tid>::StateT;
  using ResultT = typename TypeIDTrait<
      AggregationContext<kSum, value_tid>::kResultTypeID>::cpptype;

  static constexpr bool kZeroInit = true;

  static inline void InitStateUnsafe(StateT *state) {
    std::memset(state, 0, sizeof(StateT));
  }
  static inline void MergeValueUnsafe(StateT *state, const ValueT &value) {
    *state += 1;
  }
  static inline void MergeStateUnsafe(StateT *dst, const StateT &src) {
    *dst += src;
  }
  static inline void FinalizeUnsafe(ResultT *result, const StateT &state) {
    *result = state;
  }
};

template <TypeID value_tid>
struct AggregationUnsafeOps<kSum, value_tid> {
  using ValueT = typename TypeIDTrait<value_tid>::cpptype;
  using StateT = typename AggregationContext<kSum, value_tid>::StateT;
  using ResultT = typename TypeIDTrait<
      AggregationContext<kSum, value_tid>::kResultTypeID>::cpptype;

  static constexpr bool kZeroInit = true;

  static inline void InitStateUnsafe(StateT *state) {
    std::memset(state, 0, sizeof(StateT));
  }
  static inline void MergeValueUnsafe(StateT *state, const ValueT &value) {
    *state += value;
  }
  static inline void MergeStateUnsafe(StateT *dst, const StateT &src) {
    *dst += src;
  }
  static inline void FinalizeUnsafe(ResultT *result, const StateT &state) {
    *result = state;
  }
};

template <TypeID value_tid>
struct AggregationUnsafeOps<kAvg, value_tid> {
  using ValueT = typename TypeIDTrait<value_tid>::cpptype;
  using StateT = typename AggregationContext<kAvg, value_tid>::StateT;
  using ResultT = typename TypeIDTrait<
      AggregationContext<kSum, value_tid>::kResultTypeID>::cpptype;

  static constexpr bool kZeroInit = true;

  static inline void InitStateUnsafe(StateT *state) {
    std::memset(state, 0, sizeof(StateT));
  }
  static inline void MergeValueUnsafe(StateT *state, const ValueT &value) {
    state->first += value;
    state->second += 1;
  }
  static inline void MergeStateUnsafe(StateT *dst, const StateT &src) {
    dst->first += src.first;
    dst->second += src.second;
  }
  static inline void FinalizeUnsafe(ResultT *result, const StateT &state) {
    *result = static_cast<ResultT>(state.first / state.second);
  }
};


// Atomic operators
// -----------------------------------------------------------------------------
template <TypeID value_tid>
struct AggregationAtomicOps<kCount, value_tid> {
  using ValueT = typename TypeIDTrait<value_tid>::cpptype;
  using StateT = typename AggregationContext<kCount, value_tid>::StateT;
  using AtomicStateT = typename AggregationContext<kCount, value_tid>::AtomicStateT;
  using ResultT = typename TypeIDTrait<
      AggregationContext<kCount, value_tid>::kResultTypeID>::cpptype;

  static constexpr bool kZeroInit = true;

  static inline void InitStateAtomic(AtomicStateT *state) {
    state->store(0, std::memory_order_relaxed);
  }
  static inline void MergeValueAtomic(AtomicStateT *state, const ValueT &value) {
    state->fetch_add(1, std::memory_order_relaxed);
  }
  static inline void MergeStateAtomic(AtomicStateT *dst, const AtomicStateT &src) {
    dst->fetch_add(src.load(std::memory_order_relaxed),
                   std::memory_order_relaxed);
  }
  static inline void FinalizeAtomic(ResultT *result, const AtomicStateT &state) {
    *result = state.load(std::memory_order_relaxed);
  }
};

template <TypeID value_tid>
struct AggregationAtomicOps<kSum, value_tid> {
  using ValueT = typename TypeIDTrait<value_tid>::cpptype;
  using StateT = typename AggregationContext<kSum, value_tid>::StateT;
  using AtomicStateT = typename AggregationContext<kSum, value_tid>::AtomicStateT;
  using ResultT = typename TypeIDTrait<
      AggregationContext<kSum, value_tid>::kResultTypeID>::cpptype;

  static constexpr bool kZeroInit = true;

  static inline void InitStateAtomic(AtomicStateT *state) {
    state->store(0, std::memory_order_relaxed);
  }
  static inline void MergeValueAtomic(AtomicStateT *state, const ValueT &value) {
    StateT state_val = state->load(std::memory_order_relaxed);
    while (!state->compare_exchange_weak(state_val, state_val + value,
                                         std::memory_order_relaxed)) {}
  }
  static inline void MergeStateAtomic(AtomicStateT *dst, const AtomicStateT &src) {
    const StateT src_val = src.load(std::memory_order_relaxed);
    StateT dst_val = dst->load(std::memory_order_relaxed);
    while (!dst->compare_exchange_weak(dst_val, dst_val + src_val,
                                       std::memory_order_relaxed)) {}
  }
  static inline void FinalizeAtomic(ResultT *result, const AtomicStateT &state) {
    *result = state.load(std::memory_order_relaxed);
  }
};

template <TypeID value_tid>
struct AggregationAtomicIntegerSumOps {
  using ValueT = typename TypeIDTrait<value_tid>::cpptype;
  using AtomicStateT = typename AggregationContext<kSum, value_tid>::AtomicStateT;
  using ResultT = typename TypeIDTrait<
      AggregationContext<kSum, value_tid>::kResultTypeID>::cpptype;

  static constexpr bool kZeroInit = true;

  static inline void InitStateAtomic(AtomicStateT *state) {
    state->store(0, std::memory_order_relaxed);
  }
  static inline void MergeValueAtomic(AtomicStateT *state, const ValueT &value) {
    state->fetch_add(value, std::memory_order_relaxed);
  }
  static inline void MergeStateAtomic(AtomicStateT *dst, const AtomicStateT &src) {
    dst->fetch_add(src.load(std::memory_order_relaxed),
                   std::memory_order_relaxed);
  }
  static inline void FinalizeAtomic(ResultT *result, const AtomicStateT &state) {
    *result = state.load(std::memory_order_relaxed);
  }
};

template <>
struct AggregationAtomicOps<kSum, kInt> : AggregationAtomicIntegerSumOps<kInt> {};
template <>
struct AggregationAtomicOps<kSum, kLong> : AggregationAtomicIntegerSumOps<kLong> {};

template <TypeID value_tid>
struct AggregationAtomicOps<kAvg, value_tid> {
  using ValueT = typename TypeIDTrait<value_tid>::cpptype;
  using StateT = typename AggregationContext<kAvg, value_tid>::StateT;
  using AtomicStateT = typename AggregationContext<kAvg, value_tid>::AtomicStateT;
  using ResultT = typename TypeIDTrait<
      AggregationContext<kAvg, value_tid>::kResultTypeID>::cpptype;
  using SumOp = AggregationAtomicOps<kSum, value_tid>;
  using CntOp = AggregationAtomicOps<kSum, kLong>;

  static constexpr bool kZeroInit = true;

  static inline void InitStateAtomic(AtomicStateT *state) {
    SumOp::InitStateAtomic(&state->first);
    CntOp::InitStateAtomic(&state->second);
  }
  static inline void MergeValueAtomic(AtomicStateT *state, const ValueT &value) {
    SumOp::MergeValueAtomic(&state->first, value);
    CntOp::MergeValueAtomic(&state->second, 1);
  }
  static inline void MergeStateAtomic(AtomicStateT *dst, const AtomicStateT &src) {
    SumOp::MergeStateAtomic(&dst->first, src.first);
    CntOp::MergeStateAtomic(&dst->second, src.second);
  }
  static inline void FinalizeAtomic(ResultT *result, const AtomicStateT &state) {
    typename SumOp::ResultT sum;
    typename CntOp::ResultT cnt;
    SumOp::FinalizeAtomic(&sum, state.first);
    CntOp::FinalizeAtomic(&cnt, state.second);
    *result = static_cast<ResultT>(sum / cnt);
  }
};

template <TypeID value_tid>
struct AggregationAtomicOps<kHasMultipleValues, value_tid> {
  using ValueT = typename TypeIDTrait<value_tid>::cpptype;
  using StateT = typename AggregationContext<kHasMultipleValues, value_tid>::StateT;
  using AtomicStateT = typename AggregationContext<kHasMultipleValues, value_tid>::AtomicStateT;
  using ResultT = typename TypeIDTrait<
      AggregationContext<kHasMultipleValues, value_tid>::kResultTypeID>::cpptype;

  static constexpr bool kZeroInit = true;

  static inline void InitStateAtomic(AtomicStateT *state) {
    std::memset(state, 0, sizeof(AtomicStateT));
  }
  static inline void MergeValueAtomic(AtomicStateT *state, const ValueT &value) {
    StateT state_val = state->load(std::memory_order_relaxed);
    StateT desired;
    do {
      switch (state_val.status) {
        case 0:
          desired.value = value;
          desired.status = 1;
          continue;
        case 1:
          if (state_val.value == value) {
            break;
          }
          desired.value = value;
          desired.status = 2;
          continue;
        default:
          break;
      }
      break;
    } while (!state->compare_exchange_weak(state_val, desired,
                                           std::memory_order_relaxed));
  }
  static inline void MergeStateAtomic(AtomicStateT *dst, const AtomicStateT &src) {
    LOG(FATAL) << "TODO";
  }
  static inline void FinalizeAtomic(ResultT *result, const AtomicStateT &state) {
    *result = (state.load(std::memory_order_relaxed).status == 2);
  }
};

template <AggregationID agg_id>
struct MinMaxFunctor;
template <>
struct MinMaxFunctor<kMin> {
  template <typename T>
  inline static const T& Apply(const T &a, const T &b) {
    return std::min(a, b);
  }
};
template <>
struct MinMaxFunctor<kMax> {
  template <typename T>
  inline static const T& Apply(const T &a, const T &b) {
    return std::max(a, b);
  }
};

template <AggregationID agg_id, TypeID value_tid>
struct AggregationAtomicMinMaxOps {
  using ValueT = typename TypeIDTrait<value_tid>::cpptype;
  using StateT = typename AggregationContext<agg_id, value_tid>::StateT;
  using AtomicStateT = typename AggregationContext<agg_id, value_tid>::AtomicStateT;
  using ResultT = typename TypeIDTrait<
      AggregationContext<agg_id, value_tid>::kResultTypeID>::cpptype;

  using Comparator = MinMaxFunctor<agg_id>;

  static constexpr bool kZeroInit = true;

  static inline void InitStateAtomic(AtomicStateT *state) {
    std::memset(state, 0, sizeof(AtomicStateT));
  }
  static inline void MergeValueAtomic(AtomicStateT *state, const ValueT &value) {
    StateT state_val = state->load(std::memory_order_relaxed);
    StateT desired;
    do {
      desired.value = state_val.valid ? Comparator::Apply(state_val.value, value)
                                      : value;
      desired.valid = true;
    } while (!state->compare_exchange_weak(state_val, desired,
                                           std::memory_order_relaxed));
  }
  static inline void MergeStateAtomic(AtomicStateT *dst, const AtomicStateT &src) {
    LOG(FATAL) << "TODO";
  }
  static inline void FinalizeAtomic(ResultT *result, const AtomicStateT &state) {
    *result = state.load(std::memory_order_relaxed).value;
  }
};

template <TypeID value_tid>
struct AggregationAtomicOps<kMin, value_tid> : AggregationAtomicMinMaxOps<kMin, value_tid> {};
template <TypeID value_tid>
struct AggregationAtomicOps<kMax, value_tid> : AggregationAtomicMinMaxOps<kMax, value_tid> {};

/** @} */

}  // namespace quickstep

#endif  // QUICKSTEP_PROTOTYPE_AGGREGATE_FUNCTION_TRAITS_HPP_
