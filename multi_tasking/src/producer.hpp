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
#include "zpp_include/thread.hpp"

// local
#include "buffer_solution.hpp"

namespace multi_tasking {

template <typename T, class DataGenerator>
class Producer {
 public:
  explicit Producer(Buffer<T>& buffer)
      : _buffer(buffer),
        _producerThread(zpp_lib::PreemptableThreadPriority::PriorityNormal,
                        "ProducerThread") {}

  void start() {
    auto res = _producerThread.start(std::bind(&Producer::producerMethod, this));
    __ASSERT(res, "Cannot start producer thread: %d", (int)res.error());
  }

  void wait() {
    auto res = _producerThread.join();
    __ASSERT(res, "Cannot join producer thread: %d", (int)res.error());
  }

 private:
  void producerMethod() {
    while (true) {
      T producerData = DataGenerator::produceNextValue();
      uint32_t index = _buffer.append(producerData);
      std::cout << "Producer data is " << producerData << " (index in buffer " << index
                << ")" << std::endl;
    }
  }

 private:
  static constexpr std::chrono::milliseconds kProduceWaitTime = 500ms;
  Buffer<T>& _buffer;
  zpp_lib::Thread _producerThread;
};

}  // namespace multi_tasking
