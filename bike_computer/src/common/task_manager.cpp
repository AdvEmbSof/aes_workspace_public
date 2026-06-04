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
 * @file task_manager.cpp
 * @author Serge Ayer <serge.ayer@hefr.ch>
 *
 * @brief TaskManager implementation
 *
 * @date 2025-07-01
 * @version 1.0.0
 ***************************************************************************/

#include "task_manager.hpp"

// zephyr
#if CONFIG_TEST
#include <zephyr/ztest.h>
#endif // CONFIG_TEST
// #include <zephyr/tracing/tracing.h>

// std
#include <chrono>

// zpp_lib
#include "zpp_include/this_thread.hpp"
#include "zpp_include/zpp_assert.hpp"
#include "zpp_include/zpp_log.hpp"

ZPP_LOG_MODULE_DECLARE(bike_computer, CONFIG_APP_LOG_LEVEL);

namespace bike_computer {

void TaskManager::initialize_phase() {
#if CONFIG_TEST
  for (uint8_t task_index = 0; task_index < kNbrOfTaskTypes; task_index++) {
    _nbr_of_calls[task_index] = 0;
  }
  _phase = zpp_lib::Time::get_uptime();
#endif // CONFIG_TEST
}

void TaskManager::register_task_start(TaskType taskType) {
  auto task_index = static_cast<uint8_t>(taskType);
  ZPP_ASSERT(task_index < kNbrOfTaskTypes, "Invalid task index %d", task_index);
  // we assert that task_index is valid in the beginning of the method
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
  _task_start_time[task_index] = zpp_lib::Time::get_uptime();
#if CONFIG_TEST
  // we assert that task_index is valid in the beginning of the method
  _dephased_task_start_time[task_index] = _task_start_time[task_index] - _phase;
#endif // CONFIG_TEST
}

void TaskManager::simulate_computation_time(TaskType taskType, bool allowSleep) {
  auto task_index = static_cast<uint8_t>(taskType);
  ZPP_ASSERT(task_index < kNbrOfTaskTypes, "Invalid task index %d", task_index);
  // we assert that task_index is valid in the beginning of the method
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
  auto task_start_time = _task_start_time[task_index];
  auto elapsed_time    = zpp_lib::Time::get_uptime() - task_start_time;
  if (allowSleep) {
    // make sure that we still have to sleep for a while
    if (get_task_computation_time(taskType) - elapsed_time > kAllowedDelta) {
      zpp_lib::ThisThread::sleep_for(get_task_computation_time(taskType) - elapsed_time - kAllowedDelta);
    }
    // make sure that we slept long enough
    // we assert that task_index is valid in the beginning of the method
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    elapsed_time = zpp_lib::Time::get_uptime() - task_start_time;
    while (elapsed_time < get_task_computation_time(taskType)) {
      elapsed_time = zpp_lib::Time::get_uptime() - task_start_time;
    }
  } else {
    while (elapsed_time < get_task_computation_time(taskType)) {
      elapsed_time = zpp_lib::Time::get_uptime() - task_start_time;
    }
  }
#if CONFIG_TEST
  check_task_time(task_type);
  _nbr_of_calls[task_index]++;
#endif
}

#if CONFIG_TEST
void TaskManager::check_task_time(TaskType taskType) {
  uint8_t taskIndex = (uint8_t)taskType;
  __ASSERT(taskIndex < kNbrOfTaskTypes, "Invalid task index %d", taskIndex);
  std::chrono::microseconds taskComputationTime = zpp_lib::Time::get_uptime() - _task_start_time[taskIndex];
  zassert_true(taskComputationTime <= kTaskComputationTimes[taskIndex] + kAllowedDelta,
               "Task %d computation time is too large at call #%d (%lld vs %lld us, "
               "allowed delta %lld us)",
               taskIndex,
               _nbrOfCalls[taskIndex],
               taskComputationTime.count(),
               kTaskComputationTimes[taskIndex].count(),
               kAllowedDelta.count());

  // The minimum task start time is the period x nbrOfCalls
  // The minimum task start time is the period x (nbrOfCalls + 1) - task computation
  // time
  std::chrono::microseconds minDephasedTaskStartTime = kTaskPeriods[taskIndex] * _nbrOfCalls[taskIndex];
  std::chrono::microseconds maxDephasedTaskStartTime =
      kTaskPeriods[taskIndex] * (_nbrOfCalls[taskIndex] + 1) - kTaskComputationTimes[taskIndex];
  zassert_true(_dephasedTaskStartTime[taskIndex] >= minDephasedTaskStartTime - kAllowedDelta,
               "Task %s started too early at call #%d (%lld vs %lld us, allowedDelta %lld us)",
               kTaskDescriptors[taskIndex].c_str(),
               _nbrOfCalls[taskIndex],
               _dephasedTaskStartTime[taskIndex].count(),
               minDephasedTaskStartTime.count(),
               kAllowedDelta.count());
  zassert_true(_dephasedTaskStartTime[taskIndex] <= maxDephasedTaskStartTime + kAllowedDelta,
               "Task %s started too late at call #%d (%lld vs %lld us, allowedDelta %lld us)",
               kTaskDescriptors[taskIndex].c_str(),
               _nbrOfCalls[taskIndex],
               _dephasedTaskStartTime[taskIndex].count(),
               maxDephasedTaskStartTime.count(),
               kAllowedDelta.count());
}

// This method is provided for convenience.
// Suppress the exception if you use it
// cppcheck-suppress unusedFunction
bool TaskManager::isWithinExpectedTime(TaskType taskType) {
  uint8_t taskIndex        = (uint8_t)taskType;
  auto expectedTaskEndTime = kTaskPeriods[taskIndex] * (_nbrOfCalls[taskIndex] + 1);
  return (_dephasedTaskStartTime[taskIndex] + kTaskComputationTimes[taskIndex]) < expectedTaskEndTime;
}
#endif // CONFIG_TEST == 1

} // namespace bike_computer
