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
 * @file stack_overflow.hpp
 * @author Serge Ayer <serge.ayer@hefr.ch>
 *
 * @brief Declaration/Implementation of the StackOverflow class
 *
 * @date 2025-07-01
 * @version 1.0.0
 ***************************************************************************/

#pragma once

// zephyr
#include <zephyr/kernel.h>

// std
#include <cstdint>

namespace memory_demo {

class StackOverflow {
public:
  StackOverflow()  = default;
  ~StackOverflow() = default;

  // prevent copy and move operations
  StackOverflow(const StackOverflow&)             = delete;
  StackOverflow& operator=(const StackOverflow&)  = delete;
  StackOverflow(const StackOverflow&&)            = delete;
  StackOverflow& operator=(const StackOverflow&&) = delete;

  void allocate_on_stack() {
    // allocate an array with growing size until it does not fit on the stack anymore
    size_t alloc_size = kArraySize * _multiplier;
    // Create a variable-size object on the stack
    // We use a local array to allocate memory on the stack on purpose
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
    double another_array[alloc_size];
    for (size_t i = 0; i < alloc_size; i++) {
      another_array[i] = i;
    }
    // copy to member variable to prevent them from being optimized away
    for (size_t i = 0; i < kArraySize; i++) {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
      _double_array[i] += another_array[i];
    }
    _multiplier++;
  }

private:
  static constexpr size_t kArraySize = 40;
  // We use a C-style array to allocate memory on the stack on purpose
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  double _double_array[kArraySize] = {0};
  size_t _multiplier               = 1;
};

}  // namespace memory_demo
