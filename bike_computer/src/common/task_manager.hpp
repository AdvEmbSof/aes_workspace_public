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
 * @file task_manager.hpp
 * @author Serge Ayer <serge.ayer@hefr.ch>
 *
 * @brief TaskManager header file
 *
 * @date 2025-07-01
 * @version 1.0.0
 ***************************************************************************/

#pragma once

// std
#include <chrono>

// zpp_lib
#include "zpp_include/non_copyable.hpp"
#include "zpp_include/time.hpp"

namespace bike_computer {

using namespace std::literals;

class TaskManager : private zpp_lib::NonCopyable<TaskManager> {
 public:
  // TaskType definitions (YOU MUST UPDATE kNbrOfTasksTypes if you modify it)
  enum class TaskType {
    GearTaskType        = 0,
    SpeedTaskType       = 1,
    TemperatureTaskType = 2,
    ResetTaskType       = 3,
    DisplayTask1Type    = 4,
    DisplayTask2Type    = 5
  };
  static constexpr uint8_t kNbrOfTaskTypes = 6;

  TaskManager() = default;
  void initializePhase();
  void registerTaskStart(TaskType taskType);
  void simulateComputationTime(TaskType taskType);
  static inline std::chrono::microseconds getTaskComputationTime(TaskType taskType) {
    uint8_t taskIndex = (uint8_t)taskType;
    return kTaskComputationTimes[taskIndex] - kTaskOverheadTime;
  }

 private:
  // private methods
  void logPeriodAndExecutionTime(TaskType taskType);
  void logDropTask(TaskType taskType);
  bool isWithinExpectedTime(TaskType taskType);

  // constants
  static const char* kTaskDescriptors[kNbrOfTaskTypes];
  // kTaskOverheadTime accounts for additional time needed for logging between tasks
#if CONFIG_LOG == 1
  static constexpr std::chrono::microseconds kTaskOverheadTime = 13000us;
#else
  static constexpr std::chrono::microseconds kTaskOverheadTime = 5us;
#endif
  static constexpr std::chrono::microseconds kTaskComputationTimes[kNbrOfTaskTypes] = {
      100000us, 200000us, 100000us, 100000us, 200000us, 100000us};
  static constexpr std::chrono::microseconds kTaskPeriods[kNbrOfTaskTypes] = {
      800000us, 400000us, 1600000us, 800000us, 1600000us, 1600000us};
  static constexpr std::chrono::microseconds kAllowedDelta = 1000us;
  // data members
  std::chrono::microseconds _taskStartTime[kNbrOfTaskTypes]         = {0ms};
  std::chrono::microseconds _dephasedTaskStartTime[kNbrOfTaskTypes] = {0ms};
  uint32_t _nbrOfCalls[kNbrOfTaskTypes]                             = {0};
  std::chrono::microseconds _phase;
};

}  // namespace bike_computer
