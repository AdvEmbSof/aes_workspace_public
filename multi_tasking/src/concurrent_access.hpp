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
 * @file concurrent_access.hpp
 * @author Serge Ayer <serge.ayer@hefr.ch>
 *
 * @brief Declaration/Implementation of the ConcurrentAccess class
 *
 * @date 2025-07-01
 * @version 1.0.0
 ***************************************************************************/

#pragma once

// std
#include <atomic>

// zpp_lib
#include "zpp_include/thread.hpp"
#include "zpp_include/zephyr_result.hpp"

namespace multi_tasking {

class ConcurrentAccess {
public:
  explicit ConcurrentAccess(const char* threadName);

  [[nodiscard]] zpp_lib::ZephyrResult start();

  void access_shared_resources() {
    perform_concurrent_accesses(false);
  }

private:
  void perform_concurrent_accesses(bool producer);

  zpp_lib::Thread _thread;
  // simple counter
  std::atomic<uint32_t> _counter{0};
  // producer/consumer
  struct Data {
    uint32_t att1;
    uint32_t att2;
  };
  Data _shared_data{.att1 = 0, .att2 = 0};
  std::atomic<bool> _data_ready{false};
  // read and modify pattern
  Data _own_data{.att1 = 0, .att2 = 0};
  std::atomic<uint32_t> _arrived{0};
  std::atomic<uint32_t> _finished{0};
};

}  // namespace multi_tasking
