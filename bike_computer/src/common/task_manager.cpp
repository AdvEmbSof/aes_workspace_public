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
  for (uint8_t taskIndex = 0; taskIndex < kNbrOfTaskTypes; taskIndex++) {
    _nbrOfCalls[taskIndex] = 0;
  }
  _phase = zpp_lib::Time::getUpTime();
}

void TaskManager::registerTaskStart(TaskType taskType) {
  uint8_t taskIndex                 = (uint8_t)taskType;
  _taskStartTime[taskIndex]         = zpp_lib::Time::getUpTime();
  _dephasedTaskStartTime[taskIndex] = _taskStartTime[taskIndex] - _phase;
}

void TaskManager::simulateComputationTime(TaskType taskType) {
  uint8_t taskIndex = (uint8_t)taskType;
  if (isWithinExpectedTime(taskType)) {
    auto elapsedTime = zpp_lib::Time::getUpTime() - _taskStartTime[taskIndex];
    while (elapsedTime < getTaskComputationTime(taskType)) {
      elapsedTime = zpp_lib::Time::getUpTime() - _taskStartTime[taskIndex];
    }

    logTaskTime(taskType);
  } else {
    auto expectedTaskEndTime = _phase +
                               (kTaskPeriods[taskIndex] * (_nbrOfCalls[taskIndex] + 1)) -
                               kTaskOverheadTime;

    auto currentTime = zpp_lib::Time::getUpTime();
    while (currentTime < expectedTaskEndTime) {
      currentTime = zpp_lib::Time::getUpTime();
    }

    logDropTask(taskType);
  }
  _nbrOfCalls[taskIndex]++;
}

void TaskManager::logTaskTime(TaskType taskType) {
  uint8_t taskIndex = (uint8_t)taskType;
#if CONFIG_TEST == 1
  __ASSERT(taskIndex < kNbrOfTaskTypes, "Invalid task index %d", taskIndex);
  std::chrono::microseconds taskComputationTime =
      zpp_lib::Time::getUpTime() - _taskStartTime[taskIndex];
  zassert_true(taskComputationTime <= kTaskComputationTimes[taskIndex] + kAllowedDelta,
               "Task %d computation time is too large at call #%d (%lld vs %lld us)",
               taskIndex,
               _nbrOfCalls[taskIndex],
               taskComputationTime.count(),
               kTaskComputationTimes[taskIndex].count());

  // The minimum task start time is the period x nbrOfCalls
  // The minimum task start time is the period x (nbrOfCalls + 1) - task computation time
  std::chrono::microseconds minDephasedTaskStartTime =
      kTaskPeriods[taskIndex] * _nbrOfCalls[taskIndex];
  std::chrono::microseconds maxDephasedTaskStartTime =
      kTaskPeriods[taskIndex] * (_nbrOfCalls[taskIndex] + 1) -
      kTaskComputationTimes[taskIndex];
  LOG_DBG("Task %s: start time %lld (bounds %lld - %lld), computation time %lld",
          kTaskDescriptors[taskIndex],
          _dephasedTaskStartTime[taskIndex].count(),
          minDephasedTaskStartTime.count(),
          maxDephasedTaskStartTime.count(),
          taskComputationTime.count());
  zassert_true(
      _dephasedTaskStartTime[taskIndex] >= minDephasedTaskStartTime - kAllowedDelta,
      "Task %s started too early at call #%d (%lld vs %lld us)",
      kTaskDescriptors[taskIndex],
      _nbrOfCalls[taskIndex],
      _dephasedTaskStartTime[taskIndex].count(),
      minDephasedTaskStartTime.count());
  zassert_true(
      _dephasedTaskStartTime[taskIndex] <= maxDephasedTaskStartTime + kAllowedDelta,
      "Task %s started too late at call #%d (%lld vs %lld us)",
      kTaskDescriptors[taskIndex],
      _nbrOfCalls[taskIndex],
      _dephasedTaskStartTime[taskIndex].count(),
      maxDephasedTaskStartTime.count());
#else
  std::chrono::microseconds taskComputationTime =
      zpp_lib::Time::getUpTime() - _taskStartTime[taskIndex];
  std::chrono::microseconds minDephasedTaskStartTime =
      kTaskPeriods[taskIndex] * _nbrOfCalls[taskIndex];
  std::chrono::microseconds maxDephasedTaskStartTime =
      kTaskPeriods[taskIndex] * (_nbrOfCalls[taskIndex] + 1) -
      kTaskComputationTimes[taskIndex];
  sys_trace_named_event("Task end", taskIndex, 0);
  LOG_DBG("Task %s: start time %lld (bounds %lld - %lld), computation time %lld",
          kTaskDescriptors[taskIndex],
          _dephasedTaskStartTime[taskIndex].count(),
          minDephasedTaskStartTime.count(),
          maxDephasedTaskStartTime.count(),
          taskComputationTime.count());
#endif  // CONFIG_TEST == 1
}

void TaskManager::logDropTask(TaskType taskType) {
  uint8_t taskIndex = (uint8_t)taskType;
  std::chrono::microseconds minDephasedTaskStartTime =
      kTaskPeriods[taskIndex] * _nbrOfCalls[taskIndex];
  std::chrono::microseconds maxDephasedTaskStartTime =
      kTaskPeriods[taskIndex] * (_nbrOfCalls[taskIndex] + 1) -
      kTaskComputationTimes[taskIndex];
  LOG_DBG("Task %s DROPPED: start time %lld (bounds %lld - %lld), computation time %lld",
          kTaskDescriptors[taskIndex],
          _dephasedTaskStartTime[taskIndex].count(),
          minDephasedTaskStartTime.count(),
          maxDephasedTaskStartTime.count(),
          kTaskComputationTimes[taskIndex].count());
}

bool TaskManager::isWithinExpectedTime(TaskType taskType) {
  uint8_t taskIndex        = (uint8_t)taskType;
  auto expectedTaskEndTime = kTaskPeriods[taskIndex] * (_nbrOfCalls[taskIndex] + 1);
  return (_dephasedTaskStartTime[taskIndex] + kTaskComputationTimes[taskIndex]) <
         expectedTaskEndTime;
}

}  // namespace bike_computer
