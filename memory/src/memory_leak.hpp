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
 * @file memory_leak.hpp
 * @author Serge Ayer <serge.ayer@hefr.ch>
 *
 * @brief Declaration/Implementation of the MemoryLeak class
 *
 * @date 2025-07-01
 * @version 1.0.0
 ***************************************************************************/

#pragma once

// zephyr
#include <zephyr/kernel.h>

namespace memory_demo {

class MemoryLeak {
public:
  static constexpr uint16_t kArraySize = 1024;

  // create a memory leak in the constructor itself
  MemoryLeak() {
    _ptr = new uint8_t[kArraySize];
    __ASSERT(_ptr != nullptr, "Cannot allocate memory");
  }
  ~MemoryLeak() {}

  MemoryLeak(const MemoryLeak&)            = delete;
  MemoryLeak& operator=(const MemoryLeak&) = delete;

  void use() {
    for (uint16_t i = 0; i < kArraySize; i++) {
      _ptr[i] = i;
    }
  }

private:
  uint8_t* _ptr;
};

}  // namespace memory_demo
