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
 * @file car_system.hpp
 * @author Serge Ayer <serge.ayer@hefr.ch>
 *
 * @brief Car System header file
 *
 * @date 2025-07-01
 * @version 1.0.0
 ***************************************************************************/

#pragma once

// stl
#include <atomic>

// zpp_lib
#include "zpp_include/non_copyable.hpp"
#include "zpp_include/zephyr_result.hpp"
#include "zpp_include/thread.hpp"
#include "zpp_include/semaphore.hpp"

namespace car_system {

using std::literals::chrono_literals::operator""ms;

class CarSystem : private zpp_lib::NonCopyable<CarSystem> {
 public:
  // constructor
  CarSystem();

  // destructor
  ~CarSystem() = default;

  // method called in main() for starting the system
  [[nodiscard]] zpp_lib::ZephyrResult start();

  // method called for stopping the system
  void stop();

 private:
  // private methods
  // task related structures
  struct TaskInfo {
    std::chrono::milliseconds _computationTime;
    std::chrono::milliseconds _period;
    zpp_lib::PreemptableThreadPriority _priority;
    const char* _szTaskName;
  };
  // task related methods
  void task_method(const TaskInfo& taskInfo);

  // Task related data members (one thread per task)
  static constexpr uint8_t NBR_OF_TASKS = 3;
  static constexpr uint8_t TASK_INDEX_1 = 0;
  static constexpr uint8_t TASK_INDEX_2 = 1;
  static constexpr uint8_t TASK_INDEX_3 = 2;
  zpp_lib::Thread _threads[NBR_OF_TASKS];
  static constexpr TaskInfo _taskInfos[NBR_OF_TASKS] = { 
    // TODO: initialize _taskInfos based on task definitions (WCE, Period, Priority, Name)
    
  std::function<void()> _taskMethods[NBR_OF_TASKS] = { 
    // TODO: initialize _taskMethods so that each thread receives the appropriate TaskInfo
    
  };
  // Semaphore used to synchronize all threads at startup
  zpp_lib::Semaphore _startSemaphore{0, NBR_OF_TASKS};
  // stop flag, used for stopping each task (set in stop())
  volatile std::atomic<bool> _stopFlag = false;
};

}  // namespace car_system
