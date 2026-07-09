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

// std
#include <chrono>

// zpp_lib
#include "zpp_include/this_thread.hpp"
#include "zpp_include/zpp_assert.hpp"
#include "zpp_include/zpp_log.hpp"
#if CONFIG_TEST
#include "zpp_include/zpp_test.hpp"
#endif  // CONFIG_TEST

ZPP_LOG_MODULE_DECLARE(bike_computer, CONFIG_APP_LOG_LEVEL);

namespace bike_computer {

void TaskManager::initialize_phase() {
#if CONFIG_TEST || CONFIG_LOG_TASK_TIMES
  for (uint32_t& nbr_of_calls : _nbr_of_calls) {
    nbr_of_calls = 0;
  }
  _phase = zpp_lib::Time::get_uptime();
#endif  // CONFIG_TEST || CONFIG_LOG_TASK_TIMES
}

void TaskManager::register_task_start(TaskType task_type) {
  auto task_index = static_cast<uint8_t>(task_type);
  ZPP_ASSERT(task_index < kNbrOfTaskTypes, "Invalid task index %d", task_index);
  // task_type is an enum class and task_index is within bounds, so we can safely use it as an index
  // NOLINTBEGIN(cppcoreguidelines-pro-bounds-constant-array-index)
  _task_start_time[task_index] = zpp_lib::Time::get_uptime();
#if CONFIG_TEST || CONFIG_LOG_TASK_TIMES
  // we assert that task_index is valid in the beginning of the method
  _dephased_task_start_time[task_index] = _task_start_time[task_index] - _phase;
#endif  // CONFIG_TEST || CONFIG_LOG_TASK_TIMES
  // NOLINTEND(cppcoreguidelines-pro-bounds-constant-array-index)
}

void TaskManager::simulate_computation_time(TaskType task_type, bool allow_sleep) {
  auto task_index = static_cast<uint8_t>(task_type);
  ZPP_ASSERT(task_index < kNbrOfTaskTypes, "Invalid task index %d", task_index);
  // task_type is an enum class and task_index is within bounds, so we can safely use it as an index
  // NOLINTBEGIN(cppcoreguidelines-pro-bounds-constant-array-index)
  auto task_start_time = _task_start_time[task_index];
  auto elapsed_time    = zpp_lib::Time::get_uptime() - task_start_time;
  if (allow_sleep) {
    // make sure that we still have to sleep for a while
    if (get_task_computation_time(task_type) - elapsed_time > kAllowedDelta) {
      zpp_lib::ThisThread::sleep_for(get_task_computation_time(task_type) - elapsed_time - kAllowedDelta);
    }
    // make sure that we slept long enough
    // we assert that task_index is valid in the beginning of the method
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    elapsed_time = zpp_lib::Time::get_uptime() - task_start_time;
    while (elapsed_time < get_task_computation_time(task_type)) {
      elapsed_time = zpp_lib::Time::get_uptime() - task_start_time;
    }
  } else {
    while (elapsed_time < get_task_computation_time(task_type)) {
      elapsed_time = zpp_lib::Time::get_uptime() - task_start_time;
    }
  }
#if CONFIG_LOG_TASK_TIMES
  auto task_computation_time        = zpp_lib::Time::get_uptime() - task_start_time;
  auto min_dephased_task_start_time = kTaskPeriods[task_index] * _nbr_of_calls[task_index];
  auto max_dephased_task_start_time = kTaskPeriods[task_index] * (_nbr_of_calls[task_index] + 1) - kTaskComputationTimes[task_index];
  ZPP_LOG_DBG("Task %s: start time %lld (bounds %lld - %lld), computation time %lld",
              kTaskDescriptors[task_index].c_str(),
              _dephased_task_start_time[task_index].count(),
              min_dephased_task_start_time.count(),
              max_dephased_task_start_time.count(),
              task_computation_time.count());
#endif  // CONFIG_LOG_TASK_TIMES
#if CONFIG_TEST
  check_task_time(task_type);
#endif
#if CONFIG_TEST || CONFIG_LOG_TASK_TIMES
  _nbr_of_calls[task_index]++;
#endif
  // NOLINTEND(cppcoreguidelines-pro-bounds-constant-array-index)
}

#if CONFIG_TEST
void TaskManager::check_task_time(TaskType task_type) {
  auto task_index = static_cast<uint8_t>(task_type);
  // task_type is an enum class and task_index is within bounds, so we can safely use it as an index
  // NOLINTBEGIN(cppcoreguidelines-pro-bounds-constant-array-index)
  ZPP_ASSERT(task_index < kNbrOfTaskTypes, "Invalid task index %d", task_index);
  auto task_computation_time = zpp_lib::Time::get_uptime() - _task_start_time[task_index];
  zpp_zassert_true(task_computation_time <= kTaskComputationTimes[task_index] + kAllowedDelta,
                   "Task %d computation time is too large at call #%d (%lld vs %lld us, "
                   "allowed delta %lld us)",
                   task_index,
                   _nbr_of_calls[task_index],
                   task_computation_time.count(),
                   kTaskComputationTimes[task_index].count(),
                   kAllowedDelta.count());

  // The minimum task start time is the period x nbrOfCalls
  // The minimum task start time is the period x (nbrOfCalls + 1) - task computation
  // time
  auto min_dephased_task_start_time = kTaskPeriods[task_index] * _nbr_of_calls[task_index];
  auto max_dephased_task_start_time = kTaskPeriods[task_index] * (_nbr_of_calls[task_index] + 1) - kTaskComputationTimes[task_index];
  zpp_zassert_true(_dephased_task_start_time[task_index] >= min_dephased_task_start_time - kAllowedDelta,
                   "Task %s started too early at call #%d (%lld vs %lld us, allowedDelta %lld us)",
                   kTaskDescriptors[task_index].c_str(),
                   _nbr_of_calls[task_index],
                   _dephased_task_start_time[task_index].count(),
                   min_dephased_task_start_time.count(),
                   kAllowedDelta.count());
  zpp_zassert_true(_dephased_task_start_time[task_index] <= max_dephased_task_start_time + kAllowedDelta,
                   "Task %s started too late at call #%d (%lld vs %lld us, allowedDelta %lld us)",
                   kTaskDescriptors[task_index].c_str(),
                   _nbr_of_calls[task_index],
                   _dephased_task_start_time[task_index].count(),
                   max_dephased_task_start_time.count(),
                   kAllowedDelta.count());
  // NOLINTEND(cppcoreguidelines-pro-bounds-constant-array-index)
}

// This method is provided for convenience.
// Suppress the exception if you use it
// cppcheck-suppress unusedFunction
bool TaskManager::is_within_expected_time(TaskType task_type) {
  auto task_index = static_cast<uint8_t>(task_type);
  ZPP_ASSERT(task_index < kNbrOfTaskTypes, "Invalid task index %d", task_index);
  // task_type is an enum class and task_index is within bounds, so we can safely use it as an index
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
  auto expected_task_end_time = kTaskPeriods[task_index] * (_nbr_of_calls[task_index] + 1);
  // task_type is an enum class and task_index is within bounds, so we can safely use it as an index
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
  return (_dephased_task_start_time[task_index] + kTaskComputationTimes[task_index]) < expected_task_end_time;
}
#endif  // CONFIG_TEST

}  // namespace bike_computer
