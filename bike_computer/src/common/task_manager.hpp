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
#include <string>

// zpp_lib
#include "zpp_include/non_copyable.hpp"
#include "zpp_include/time.hpp"

namespace bike_computer {

using std::literals::chrono_literals::operator""ms;
using std::literals::chrono_literals::operator""us;

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
  void initialize_phase();
  void register_task_start(TaskType task_type);
  void simulate_computation_time(TaskType task_type, bool allow_sleep);
  static inline std::chrono::microseconds get_task_computation_time(TaskType task_type) {
    uint8_t task_index = static_cast<uint8_t>(task_type);
    return kTaskComputationTimes[task_index] - kTaskOverheadTime;
  }

private:
  // private
#if CONFIG_TEST
  void check_task_time(TaskType taskType);
  bool is_within_expected_time(TaskType taskType);
#endif // CONFIG_TEST

  // constants
  static constexpr std::string kTaskDescriptors[kNbrOfTaskTypes] = {"Gear", "Speed", "Temperature", "Reset", "Display(1)", "Display(2)"};
  // kTaskOverheadTime accounts for additional time needed for switching between tasks
  static constexpr std::chrono::microseconds kTaskOverheadTime                      = 1us;
  static constexpr std::chrono::microseconds kTaskComputationTimes[kNbrOfTaskTypes] = {
      100000us, 200000us, 100000us, 100000us, 200000us, 100000us};
  static constexpr std::chrono::microseconds kTaskPeriods[kNbrOfTaskTypes] = {
      800000us, 400000us, 1600000us, 800000us, 1600000us, 1600000us};

  // set the allowed delta to be 100 ticks (100'000'000 over the number of ticks per
  // sec)
  static constexpr std::chrono::microseconds kAllowedDelta = std::chrono::microseconds(100000000 / CONFIG_SYS_CLOCK_TICKS_PER_SEC);

  // data members
  std::chrono::microseconds _task_start_time[kNbrOfTaskTypes] = {0ms};
#if CONFIG_TEST
  std::chrono::microseconds _dephased_task_start_time[kNbrOfTaskTypes] = {0ms};
  uint32_t _nbr_of_calls[kNbrOfTaskTypes]                              = {0};
  std::chrono::microseconds _phase;
#endif // CONFIG_TEST
};

} // namespace bike_computer
