// Copyright 2025 Haute école d'ingénierie et d'architecture de Fribourg
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/****************************************************************************
 * @file memory_fragmenter.hpp
 * @author Serge Ayer <serge.ayer@hefr.ch>
 *
 * @brief Declaration/Implementation of the MemoryFragmenter class
 *
 * @date 2025-07-01
 * @version 1.0.0
 ***************************************************************************/

#pragma once

// zephyr
#include <zephyr/kernel.h>

// zpp_lib
#include "zpp_include/utils.hpp"
#include "zpp_include/zpp_assert.hpp"

extern "C" {
// Zephyr defines this symbol globally
// To access it you need to define CONFIG_HEAP_MEM_POOL_SIZE=...
// NOLINTNEXTLINE(readability-identifier-naming) -- Zephyr defines this symbol globally
extern struct sys_heap _system_heap;
}

namespace memory_demo {

// This demo is about using heap memory
// NOLINTBEGIN(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
// NOLINTBEGIN(cppcoreguidelines-owning-memory,cppcoreguidelines-pro-bounds-constant-array-index)

class MemoryFragmenter {
public:
  // default constructor and destructor
  MemoryFragmenter()  = default;
  ~MemoryFragmenter() = default;

  // prevent copy and move operations
  MemoryFragmenter(const MemoryFragmenter&)             = delete;
  MemoryFragmenter& operator=(const MemoryFragmenter&)  = delete;
  MemoryFragmenter(const MemoryFragmenter&&)            = delete;
  MemoryFragmenter& operator=(const MemoryFragmenter&&) = delete;

  void fragment_memory() {
    // log heap info
    zpp_lib::Utils::log_heap_summary();

    // get heap available size
    struct sys_memory_stats stats = {};
    sys_heap_runtime_stats_get(&_system_heap, &stats);

    // divide the available size by 8 blocks that we allocate
    uint32_t block_size = (stats.free_bytes - kMarginSpace) / kNbrOfBlocks;
    printk("Allocating blocks of size %" PRIu32 "\n", block_size);
    char* p_block_array[kNbrOfBlocks] = {nullptr};
    for (uint32_t block_index = 0; block_index < kNbrOfBlocks; block_index++) {
      p_block_array[block_index] = new char[block_size]();
      ZPP_ASSERT(p_block_array[block_index] != nullptr, "Allocation of block %d of size %d failed", block_index, block_size);
      printk("Allocated block index  %" PRIu32 " of size  %" PRIu32 " at address %p\n",
             block_index,
             block_size,
             static_cast<void*>(p_block_array[block_index]));
      // copy to member variable to prevent them from being optimized away
      for (uint32_t index = 0; index < kArraySize; index++) {
        _double_array[index] += static_cast<double>(p_block_array[block_index][index]);
      }
    }

    // the full heap (or almost) should be allocated
    printk("Heap statistics after full allocation:\n");
    zpp_lib::Utils::log_heap_summary();

    // delete only the even blocks
    for (uint32_t block_index = 0; block_index < kNbrOfBlocks; block_index += 2) {
      delete[] p_block_array[block_index];
      p_block_array[block_index] = nullptr;
    }
    // we should have half of the heap space free
    printk("Heap statistics after half deallocation:\n");
    zpp_lib::Utils::log_heap_summary();

    // trying to allocate one block of initial size
    // it will succeed
    printk("Allocating 1 block of size %" PRIu32 " succeeds !\n", block_size);
    p_block_array[0] = new char[block_size];
    ZPP_ASSERT(p_block_array[0] != nullptr, "Allocation of block of size %d failed", block_size);

    printk("Heap statistics after allocating one more block of size %d:\n", block_size);
    zpp_lib::Utils::log_heap_summary();
    // trying to allocated one block that is slightly bigger
    // without fragmentation, this allocation should succeed
    // but it will fail...
    block_size += kBlockSizeIncrement;
    // this allocation will fail
    printk("Allocating 1 block of size %" PRIu32 " should succeed !\n", block_size);
    p_block_array[1] = new char[block_size];
    ZPP_ASSERT(p_block_array[1] != nullptr, "Allocation of block of size %d failed", block_size);

    // copy to member variable to prevent them from being optimized away
    for (uint32_t index = 0; index < kArraySize; index++) {
      _double_array[index] += static_cast<double>(p_block_array[0][index] + p_block_array[1][index]);
    }
  }

private:
  static constexpr uint8_t kNbrOfBlocks        = 8;
  static constexpr uint8_t kBlockSizeIncrement = 8;
  static constexpr uint16_t kMarginSpace       = 1024;
  static constexpr uint8_t kArraySize          = 100;
  double _double_array[kArraySize]             = {0};
};

// NOLINTEND(cppcoreguidelines-owning-memory,cppcoreguidelines-pro-bounds-constant-array-index)
// NOLINTEND(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)

}  // namespace memory_demo
