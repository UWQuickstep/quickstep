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

#ifndef QUICKSTEP_STORAGE_COLLISION_FREE_AGGREGATION_STATE_HASH_TABLE_HPP_
#define QUICKSTEP_STORAGE_COLLISION_FREE_AGGREGATION_STATE_HASH_TABLE_HPP_

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

#include "catalog/CatalogTypedefs.hpp"
#include "expressions/aggregation/AggregateFunctionTraits.hpp"
#include "expressions/aggregation/AggregationID.hpp"
#include "storage/HashTableBase.hpp"
#include "storage/StorageBlob.hpp"
#include "storage/StorageConstants.hpp"
#include "storage/ValueAccessorMultiplexer.hpp"
#include "types/Type.hpp"
#include "types/TypeID.hpp"
#include "types/containers/ColumnVector.hpp"
#include "utility/BarrieredReadWriteConcurrentBitVector.hpp"
#include "utility/Macros.hpp"

#include "glog/logging.h"

namespace quickstep {

class AggregationHandle;
class StorageManager;

/** \addtogroup Storage
 *  @{
 */

class CollisionFreeVectorTable : public AggregationStateHashTableBase {
 public:
  /**
   * @brief Constructor.
   *
   * @param key_type The group-by key type.
   * @param num_entries The estimated number of entries this table will hold.
   * @param handles The aggregation handles.
   * @param storage_manager The StorageManager to use (a StorageBlob will be
   *        allocated to hold this table's contents).
   **/
  CollisionFreeVectorTable(
      const Type *key_type,
      const std::size_t num_entries,
      const std::vector<AggregationHandle*> &handles,
      StorageManager *storage_manager);

  ~CollisionFreeVectorTable() override;

  HashTableImplType getImplType() const override {
    return HashTableImplType::kCollisionFreeVector;
  }

  void destroyPayload() override;

  /**
   * @brief Get the number of partitions to be used for initializing the table.
   *
   * @return The number of partitions to be used for initializing the table.
   */
  inline std::size_t getNumInitializationPartitions() const {
    return num_init_partitions_;
  }

  /**
   * @brief Get the number of partitions to be used for finalizing the aggregation.
   *
   * @return The number of partitions to be used for finalizing the aggregation.
   */
  inline std::size_t getNumFinalizationPartitions() const {
    return num_finalize_partitions_;
  }

  /**
   * @brief Get the exact number of tuples in the specified finalization partition.
   *
   * @return The exact number of tuples in the specified finalization partition.
   */
  inline std::size_t getNumTuplesInFinalizationPartition(
      const std::size_t partition_id) const {
    const std::size_t start_position =
        calculatePartitionStartPosition(partition_id);
    const std::size_t end_position =
        calculatePartitionEndPosition(partition_id);
    return existence_map_->onesCountInRange(start_position, end_position);
  }

  /**
   * @brief Get the existence map for this vector table.
   *
   * @return The existence map for this vector table.
   */
  inline BarrieredReadWriteConcurrentBitVector* getExistenceMap() const {
    return existence_map_.get();
  }

  /**
   * @brief Initialize the specified partition of this aggregation table.
   *
   * @param partition_id ID of the partition to be initialized.
   */
  inline void initialize(const std::size_t partition_id) {
    const std::size_t memory_segment_size =
        (memory_size_ + num_init_partitions_ - 1) / num_init_partitions_;
    const std::size_t memory_start = memory_segment_size * partition_id;
    std::memset(reinterpret_cast<char *>(blob_->getMemoryMutable()) + memory_start,
                0,
                std::min(memory_segment_size, memory_size_ - memory_start));
  }

  /**
   * @brief Use aggregation handles to update (multiple) aggregation states in
   *        this vector table, with group-by keys and arguments drawn from the
   *        given ValueAccessors.
   *
   * @param argument_ids The multi-source attribute IDs of each argument
   *        component to be read from \p accessor_mux.
   * @param key_ids The multi-source attribute IDs of each group-by key
   *        component to be read from \p accessor_mux.
   * @param accessor_mux A ValueAccessorMultiplexer object that contains the
   *        ValueAccessors which will be used to access keys. beginIteration()
   *        should be called on the accessors before calling this method.
   * @return Always return true.
   **/
  bool upsertValueAccessorCompositeKey(
      const std::vector<std::vector<MultiSourceAttributeId>> &argument_ids,
      const std::vector<MultiSourceAttributeId> &key_ids,
      const ValueAccessorMultiplexer &accessor_mux) override;

  /**
   * @brief Copy the keys from this table to a NativeColumnVector, for the
   *        specified partition.
   *
   * @param partition_id ID of the partition to copy keys from.
   * @param output_cv The NativeColumnVector to copy keys to.
   */
  void finalizeKey(const std::size_t partition_id,
                   NativeColumnVector *output_cv) const;


  /**
   * @brief Finalize the aggregation states to a NativeColumnVector, for the
   *        specified partition and aggregation handle.
   *
   * @param partition_id ID of the partition to finalize.
   * @param handle_id ID of the aggregation handle to finalize.
   * @param output_cv The NativeColumnVector to write finalized values to.
   */
  void finalizeState(const std::size_t partition_id,
                     const std::size_t handle_id,
                     NativeColumnVector *output_cv) const;

  std::size_t getMemoryConsumptionBytes() const override {
    return memory_size_;
  }

 private:
  inline static std::size_t CacheLineAlignedBytes(const std::size_t actual_bytes) {
    return (actual_bytes + kCacheLineBytes - 1) / kCacheLineBytes * kCacheLineBytes;
  }

  inline static std::size_t CalculateNumInitializationPartitions(
      const std::size_t memory_size) {
    // Set initialization memory block size as 4MB.
    constexpr std::size_t kInitBlockSize = 4uL * 1024u * 1024u;

    // At least 1 partition, at most 80 partitions.
    // TODO(jianqiao): set the upbound as (# of workers * 2) instead of the
    // hardcoded 80.
    return std::max(1uL, std::min(memory_size / kInitBlockSize, 80uL));
  }

  inline static std::size_t CalculateNumFinalizationPartitions(
      const std::size_t num_entries) {
    // Set finalization segment size as 4096 entries.
    constexpr std::size_t kFinalizeSegmentSize = 4uL * 1024L;

    // At least 1 partition, at most 80 partitions.
    // TODO(jianqiao): set the upbound as (# of workers * 2) instead of the
    // hardcoded 80.
    return std::max(1uL, std::min(num_entries / kFinalizeSegmentSize, 80uL));
  }

  inline std::size_t calculatePartitionLength() const {
    const std::size_t partition_length =
        (num_entries_ + num_finalize_partitions_ - 1) / num_finalize_partitions_;
    DCHECK_GE(partition_length, 0u);
    return partition_length;
  }

  inline std::size_t calculatePartitionStartPosition(
      const std::size_t partition_id) const {
    return calculatePartitionLength() * partition_id;
  }

  inline std::size_t calculatePartitionEndPosition(
      const std::size_t partition_id) const {
    return std::min(calculatePartitionLength() * (partition_id + 1),
                    num_entries_);
  }

  template <bool is_key_nullable, typename KeyT, typename KeyAccessorT>
  inline void upsertValueAccessorKeyOnly(const attribute_id key_attr_id,
                                         KeyAccessorT *key_accessor);

  template <bool is_key_nullable, typename KeyT, typename KeyAccessorT>
  inline void upsertValueAccessorCountNullary(const attribute_id key_attr_id,
                                              void *vec_table,
                                              KeyAccessorT *key_accessor);

  template <typename AggFunc, typename KeyT, bool is_key_nullable,
            bool is_argument_nullable, bool use_two_accessors,
            typename KeyAccessorT, typename ArgumentAccessorT>
  inline void upsertValueAccessorInternal(const attribute_id key_attr_id,
                                          const attribute_id argument_id,
                                          void *vec_table,
                                          KeyAccessorT *key_accessor,
                                          ArgumentAccessorT *argument_accessor);

  template <typename KeyT>
  inline void finalizeKeyInternal(const std::size_t start_position,
                                  const std::size_t end_position,
                                  NativeColumnVector *output_cv) const;

  template <typename AggFunc>
  inline void finalizeStateInternal(const void *vec_table,
                                    const std::size_t start_position,
                                    const std::size_t end_position,
                                    NativeColumnVector *output_cv) const;

  const Type *key_type_;
  const std::size_t num_entries_;

  const std::size_t num_handles_;
  const std::vector<AggregationHandle *> handles_;

  std::unique_ptr<BarrieredReadWriteConcurrentBitVector> existence_map_;
  std::vector<void *> vec_tables_;

  const std::size_t num_finalize_partitions_;

  StorageManager *storage_manager_;
  MutableBlobReference blob_;

  std::size_t memory_size_;
  std::size_t num_init_partitions_;

  DISALLOW_COPY_AND_ASSIGN(CollisionFreeVectorTable);
};

// ----------------------------------------------------------------------------
// Implementations of template methods follow.

template <bool is_key_nullable, typename KeyT, typename KeyAccessorT>
inline void CollisionFreeVectorTable
    ::upsertValueAccessorKeyOnly(const attribute_id key_attr_id,
                                 KeyAccessorT *accessor) {
  accessor->beginIteration();
  while (accessor->next()) {
    const KeyT *key = static_cast<const KeyT *>(
        accessor->template getUntypedValue<is_key_nullable>(key_attr_id));
    if (is_key_nullable && key == nullptr) {
      continue;
    }
    existence_map_->setBit(*key);
  }
}

template <bool is_key_nullable, typename KeyT, typename KeyAccessorT>
inline void CollisionFreeVectorTable
    ::upsertValueAccessorCountNullary(const attribute_id key_attr_id,
                                      void *vec_table,
                                      KeyAccessorT *accessor) {
  std::atomic<std::int64_t> *states =
      static_cast<std::atomic<std::int64_t>*>(vec_table);
  accessor->beginIteration();
  while (accessor->next()) {
    const KeyT *key = static_cast<const KeyT *>(
        accessor->template getUntypedValue<is_key_nullable>(key_attr_id));
    if (is_key_nullable && key == nullptr) {
      continue;
    }
    const std::size_t loc = *key;
    states[loc].fetch_add(1, std::memory_order_relaxed);
    existence_map_->setBit(loc);
  }
}

template <typename AggFunc, typename KeyT, bool is_key_nullable,
          bool is_argument_nullable, bool use_two_accessors,
          typename KeyAccessorT, typename ArgumentAccessorT>
inline void CollisionFreeVectorTable
    ::upsertValueAccessorInternal(const attribute_id key_attr_id,
                                  const attribute_id argument_id,
                                  void *vec_table,
                                  KeyAccessorT *key_accessor,
                                  ArgumentAccessorT *argument_accessor) {
  using ValueT = typename AggFunc::ValueType;
  using StateT = typename AggFunc::StateType;

  StateT *states = static_cast<StateT*>(vec_table);
  key_accessor->beginIteration();
  if (use_two_accessors) {
    argument_accessor->beginIteration();
  }
  while (key_accessor->next()) {
    if (use_two_accessors) {
      argument_accessor->next();
    }
    const KeyT *key = static_cast<const KeyT *>(
        key_accessor->template getUntypedValue<is_key_nullable>(key_attr_id));
    if (is_key_nullable && key == nullptr) {
      continue;
    }

    const std::size_t loc = *key;
    existence_map_->setBit(loc);

    const ValueT *argument = static_cast<const ValueT*>(
        argument_accessor->template getUntypedValue<is_argument_nullable>(argument_id));
    if (is_argument_nullable && argument == nullptr) {
      continue;
    }
    AggFunc::MergeValue(states + loc, *argument);
  }
}

template <typename KeyT>
inline void CollisionFreeVectorTable
    ::finalizeKeyInternal(const std::size_t start_position,
                          const std::size_t end_position,
                          NativeColumnVector *output_cv) const {
  std::size_t loc = start_position - 1;
  while ((loc = existence_map_->nextOne(loc)) < end_position) {
    *static_cast<KeyT *>(output_cv->getPtrForDirectWrite()) = loc;
  }
}

template <typename AggFunc>
inline void CollisionFreeVectorTable
    ::finalizeStateInternal(const void *vec_table,
                            const std::size_t start_position,
                            const std::size_t end_position,
                            NativeColumnVector *output_cv) const {
  using StateT = typename AggFunc::StateType;
  using ResultT = typename AggFunc::ResultType;
  const StateT *states = static_cast<const StateT*>(vec_table);

  std::size_t loc = start_position - 1;
  while ((loc = existence_map_->nextOne(loc)) < end_position) {
    AggFunc::FinalizeState(
        static_cast<ResultT*>(output_cv->getPtrForDirectWrite()),
        states[loc]);
  }
}

}  // namespace quickstep

#endif  // QUICKSTEP_STORAGE_COLLISION_FREE_AGGREGATION_STATE_HASH_TABLE_HPP_
