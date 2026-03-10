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
#include <zephyr/logging/log.h>
#if CONFIG_TEST == 1
#include <zephyr/ztest.h>
#endif  // CONFIG_TEST == 1
#include <zephyr/tracing/tracing.h>

// std
#include <chrono>

// zpp_lib
#include "zpp_include/this_thread.hpp"

LOG_MODULE_DECLARE(bike_computer, CONFIG_APP_LOG_LEVEL);

namespace bike_computer {

const char* TaskManager::kTaskDescriptors[TaskManager::kNbrOfTaskTypes] = {
    const_cast<char*>("Gear"),
    const_cast<char*>("Speed"),
    const_cast<char*>("Temperature"),
    const_cast<char*>("Reset"),
    const_cast<char*>("Display(1)"),
    const_cast<char*>("Display(2)")};

void TaskManager::initializePhase() {
#if CONFIG_TEST == 1
  for (uint8_t taskIndex = 0; taskIndex < kNbrOfTaskTypes; taskIndex++) {
    _nbrOfCalls[taskIndex] = 0;
  }
  _phase = zpp_lib::Time::getUpTime();
#endif
}

void TaskManager::registerTaskStart(TaskType taskType) {
  uint8_t taskIndex         = (uint8_t)taskType;
  _taskStartTime[taskIndex] = zpp_lib::Time::get_uptime();
#if CONFIG_TEST == 1
  _dephasedTaskStartTime[taskIndex] = _taskStartTime[taskIndex] - _phase;
#endif
}

void TaskManager::simulateComputationTime(TaskType taskType, bool allowSleep) {
  uint8_t taskIndex = (uint8_t)taskType;
  auto elapsedTime  = zpp_lib::Time::get_uptime() - _taskStartTime[taskIndex];
  if (allowSleep) {
    // make sure that we still have to sleep for a while
    if (getTaskComputationTime(taskType) - elapsedTime > kAllowedDelta) {
      zpp_lib::ThisThread::sleep_for(getTaskComputationTime(taskType) - elapsedTime -
                                     kAllowedDelta);
    }
    // make sure that we slept long enough
    elapsedTime = zpp_lib::Time::get_uptime() - _taskStartTime[taskIndex];
    while (elapsedTime < getTaskComputationTime(taskType)) {
      elapsedTime = zpp_lib::Time::get_uptime() - _taskStartTime[taskIndex];
    }
  } else {
    while (elapsedTime < getTaskComputationTime(taskType)) {
      elapsedTime = zpp_lib::Time::get_uptime() - _taskStartTime[taskIndex];
    }
  }
#if CONFIG_TEST == 1
  checkTaskTime(taskType);
  _nbrOfCalls[taskIndex]++;
#endif
}

#if CONFIG_TEST == 1
void TaskManager::checkTaskTime(TaskType taskType) {
  uint8_t taskIndex = (uint8_t)taskType;
  __ASSERT(taskIndex < kNbrOfTaskTypes, "Invalid task index %d", taskIndex);
  std::chrono::microseconds taskComputationTime =
      zpp_lib::Time::getUpTime() - _taskStartTime[taskIndex];
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
  std::chrono::microseconds minDephasedTaskStartTime =
      kTaskPeriods[taskIndex] * _nbrOfCalls[taskIndex];
  std::chrono::microseconds maxDephasedTaskStartTime =
      kTaskPeriods[taskIndex] * (_nbrOfCalls[taskIndex] + 1) -
      kTaskComputationTimes[taskIndex];
  zassert_true(
      _dephasedTaskStartTime[taskIndex] >= minDephasedTaskStartTime - kAllowedDelta,
      "Task %s started too early at call #%d (%lld vs %lld us, allowedDelta %lld us)",
      kTaskDescriptors[taskIndex],
      _nbrOfCalls[taskIndex],
      _dephasedTaskStartTime[taskIndex].count(),
      minDephasedTaskStartTime.count(),
      kAllowedDelta.count());
  zassert_true(
      _dephasedTaskStartTime[taskIndex] <= maxDephasedTaskStartTime + kAllowedDelta,
      "Task %s started too late at call #%d (%lld vs %lld us, allowedDelta %lld us)",
      kTaskDescriptors[taskIndex],
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
  return (_dephasedTaskStartTime[taskIndex] + kTaskComputationTimes[taskIndex]) <
         expectedTaskEndTime;
}
#endif  // CONFIG_TEST == 1

}  // namespace bike_computer
