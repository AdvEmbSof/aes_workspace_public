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
 * @file buffer.hpp
 * @author Serge Ayer <serge.ayer@hefr.ch>
 *
 * @brief Declaration/Implementation of the Producer class
 *
 * @date 2025-07-01
 * @version 1.0.0
 ***************************************************************************/
#pragma once

// zephyr
#include <zephyr/random/random.h>

// stl
#include <iostream>

// zpp_lib
#include "zpp_include/non_copyable.hpp"
#include "zpp_include/thread.hpp"
#include "zpp_include/zpp_assert.hpp"

// local
#include "buffer_solution.hpp"

namespace multi_tasking {

template <typename T, class DataGenerator> class Producer : public zpp_lib::NonCopyable {
public:
  explicit Producer(Buffer<T>& buffer)
      : _buffer(buffer), _producer_thread(zpp_lib::PreemptableThreadPriority::PriorityNormal, "ProducerThread") {}

  void start() {
    auto res = _producer_thread.start([this] { producer_method(); });
    ZPP_ASSERT(res, "Cannot start producer thread: %d", (int)res.error());
  }

  void wait() {
    auto res = _producer_thread.join();
    ZPP_ASSERT(res, "Cannot join producer thread: %d", (int)res.error());
  }

private:
  void producer_method() {
    while (true) {
      T producer_data = DataGenerator::s_produce_next_value();
      uint32_t index  = _buffer.append(producer_data);
      std::cout << "Producer data is " << producer_data << " (index in buffer " << index << ")" << std::endl;
    }
  }

  static constexpr std::chrono::milliseconds kProduceWaitTime = 500ms;
  Buffer<T>& _buffer;
  zpp_lib::Thread _producer_thread;
};

}  // namespace multi_tasking
