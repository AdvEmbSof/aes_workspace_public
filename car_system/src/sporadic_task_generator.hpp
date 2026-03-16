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
 * @file task_recorder.hpp
 * @author Serge Ayer <serge.ayer@hefr.ch>
 *
 * @brief SporadicTaskGenerator class declaration
 *
 * @date 2025-07-01
 * @version 1.0.0
 ***************************************************************************/

#pragma once

#if CONFIG_PHASE_B

// stl
#include <atomic>

// zpp_lib
#include "zpp_include/non_copyable.hpp"
#include "zpp_include/zephyr_result.hpp"
#include "zpp_include/message_queue.hpp"
#include "zpp_include/barrier.hpp"

namespace car_system {

class SporadicTaskGenerator : private zpp_lib::NonCopyable<SporadicTaskGenerator> {
 public:
  // constructor
  SporadicTaskGenerator() = default;

  // destructor
  ~SporadicTaskGenerator() = default;
  
  // method called from CarSystem::start() for starting generation of sporadic events
  void start(zpp_lib::Barrier& barrier);
 
  // method called for stopping the generator
  void stop();

  // method called for stopping the generator
  zpp_lib::ZephyrBoolResult getTask(std::chrono::milliseconds& taskComputationTime, const std::chrono::milliseconds& timeOut);

 private:
  // stop flag, used for stopping each task (set in stop())
  volatile std::atomic<bool> _stopFlag = false;

  static constexpr uint8_t MESSAGE_QUEUE_SIZE = 10;   
  zpp_lib::MessageQueue<std::chrono::milliseconds, MESSAGE_QUEUE_SIZE> _messageQueue;
};

}  // namespace car_system

#endif // CONFIG_PHASE_B