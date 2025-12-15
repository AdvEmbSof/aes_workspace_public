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

extern "C" {
// Zephyr defines this symbol globally
// To access it you need to define CONFIG_HEAP_MEM_POOL_SIZE=...
extern struct sys_heap _system_heap;
}

namespace memory_demo {

class MemoryFragmenter {
 public:
  // create a memory leak in the constructor itself
  MemoryFragmenter() {}

  void fragmentMemory() {
    // log heap info
    zpp_lib::Utils::logHeapSummary();

    // get heap available size
    struct sys_memory_stats stats;
    sys_heap_runtime_stats_get(&_system_heap, &stats);

    // divide the available size by 8 blocks that we allocate
    uint32_t blockSize = (stats.free_bytes - kMarginSpace) / kNbrOfBlocks;
    printk("Allocating blocks of size %" PRIu32 "\n", blockSize);
    char* pBlockArray[kNbrOfBlocks] = {NULL};
    for (uint32_t blockIndex = 0; blockIndex < kNbrOfBlocks; blockIndex++) {
      pBlockArray[blockIndex] = new char[blockSize];
      __ASSERT(pBlockArray[blockIndex] != nullptr,
               "Allocation of block %d of size %d failed",
               blockIndex,
               blockSize);
      printk("Allocated block index  %" PRIu32 " of size  %" PRIu32
             " at address 0x%08" PRIx32 "\n",
             blockIndex,
             blockSize,
             static_cast<uint32_t>(*pBlockArray[blockIndex]));
      // copy to member variable to prevent them from being optimized away
      for (uint32_t index = 0; index < kArraySize; index++) {
        _doubleArray[index] += static_cast<double>(pBlockArray[blockIndex][index]);
      }
    }

    // the full heap (or almost) should be allocated
    printk("Heap statistics after full allocation:\n");
    zpp_lib::Utils::logHeapSummary();

    // delete only the even blocks
    for (uint32_t blockIndex = 0; blockIndex < kNbrOfBlocks; blockIndex += 2) {
      delete[] pBlockArray[blockIndex];
      pBlockArray[blockIndex] = NULL;
    }
    // we should have half of the heap space free
    printk("Heap statistics after half deallocation:\n");
    zpp_lib::Utils::logHeapSummary();

    // trying to allocate one block of initial size
    // it will succeed
    printk("Allocating 1 block of size %" PRIu32 " succeeds !\n", blockSize);
    pBlockArray[0] = new char[blockSize];
    __ASSERT(
        pBlockArray[0] != nullptr, "Allocation of block of size %d failed", blockSize);

    printk("Heap statistics after allocating one more block of size %d:\n", blockSize);
    zpp_lib::Utils::logHeapSummary();
    // trying to allocated one block that is slightly bigger
    // without fragmentation, this allocation should succeed
    // but it will fail...
    blockSize += 8;
    // this allocation will fail
    printk("Allocating 1 block of size %" PRIu32 " should succeed !\n", blockSize);
    pBlockArray[1] = new char[blockSize];
    __ASSERT(
        pBlockArray[1] != nullptr, "Allocation of block of size %d failed", blockSize);

    // copy to member variable to prevent them from being optimized away
    for (uint32_t index = 0; index < kArraySize; index++) {
      _doubleArray[index] +=
          static_cast<double>(pBlockArray[0][index] + pBlockArray[1][index]);
    }
  }

 private:
  static constexpr uint8_t kNbrOfBlocks  = 8;
  static constexpr uint16_t kMarginSpace = 1024;
  static constexpr uint8_t kArraySize    = 100;
  double _doubleArray[kArraySize]        = {0};
};

}  // namespace memory_demo
