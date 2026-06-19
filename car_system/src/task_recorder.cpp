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
 * @file task_recorder.cpp
 * @author Serge Ayer <serge.ayer@hefr.ch>
 *
 * @brief TaskRecorder implementation
 *
 * @date 2025-07-01
 * @version 1.0.0
 ***************************************************************************/

#if CONFIG_TEST == 1

#include "task_recorder.hpp"

// zephyr
#include <zephyr/ztest.h>

// zpp_lib
#include "zpp_include/time.hpp"

// local
#include "userspace/init_domain.hpp"

namespace car_system {

APP_DATA std::chrono::microseconds TaskRecorder::_zeroTime = 0us;

TaskRecorder::TaskRecorder(const PeriodicTaskInfo& taskInfo) : _taskInfo(taskInfo) {}

void TaskRecorder::set_zero_time(const std::chrono::microseconds& zeroTime) {
  _zeroTime = zeroTime;
}

void TaskRecorder::start() {
  // stop() must be called before calling start() again
  __ASSERT(_stopCalled, "stop() must be called before calling start() again");

  // get the wall clock start time
  std::chrono::microseconds startTime = zpp_lib::Time::get_uptime() - _zeroTime;

  // get the CPU execution time - excludes preemption
#if !CONFIG_USERSPACE
  struct k_thread_runtime_stats stats = {};
  k_thread_runtime_stats_get(k_current_get(), &stats);
  _executionStartCycles = stats.execution_cycles;
#endif  // ! CONFIG_USERSPACE

  // detect timing violations
  SlotTimingInfo slotTimingInfo = get_slot_timing(_slotIndex);

  // test whether a slot was missed
  // If no task executed in the previous slot, then _slotIndex was not incremented.
  // In this case, startTime is larger than the previous slot end time
  if (startTime >= slotTimingInfo.endTime) {
    while (startTime >= (_slotIndex + 1) * _taskInfo._period) {
      TimingViolation violation = TimingViolation::SLOT_MISSING;
      if (has_violation(_slotIndex)) {
        update_violation_timing_violation(_slotIndex, TimingViolation::SLOT_MISSING);
      } else {
        ViolationInfo violationInfo{._timingViolation = violation, ._slotIndex = _slotIndex};
        insert_violation(violationInfo);
      }
      _slotIndex++;
    }
  } else {
    // Test whether this task does start multiple times in the same slot.
    // Ensure no underflow occurs by checking slotStartime > kPeriodOffsetTolerance
    if (slotTimingInfo.startTime > kPeriodOffsetTolerance && startTime < slotTimingInfo.startTime - kPeriodOffsetTolerance) {
      // _slotIndex may have been incremented in stop(), check whether it needs to be
      // recomputed
      uint32_t expectedSlotIndex = startTime / _taskInfo._period;
      if (expectedSlotIndex < _slotIndex) {
        // task fired early — belongs to previous slot
        _slotIndex = expectedSlotIndex;
      }

      if (has_violation(_slotIndex)) {
        update_violation_timing_violation(_slotIndex, TimingViolation::SLOT_MULTIPLE_FIRE);
      } else {
        const ViolationInfo violationInfo{
            ._timingViolation = TimingViolation::SLOT_MULTIPLE_FIRE, ._slotIndex = _slotIndex, ._taskStartTime = startTime};
        insert_violation(violationInfo);
      }
    }
  }

  // update _stopCalled
  _stopCalled = false;
}

void TaskRecorder::stop() {
  // get the wall clock stop time
  std::chrono::microseconds stopTime = zpp_lib::Time::get_uptime() - _zeroTime;

  // compute execution times, accounting for possible preemption
  std::chrono::microseconds executionTime = compute_execution_time();

  // first detect execution overshoot
  auto computationTime = get_expected_computation_time();
  if (executionTime > computationTime + kComputationTimeOffsetTolerance) {
    if (has_violation(_slotIndex)) {
      update_violation_timing_violation(_slotIndex, TimingViolation::SLOT_EXEC_OVERSHOOT);
      update_violation_execution_time(_slotIndex, executionTime);
    } else {
      ViolationInfo violationInfo{
          ._timingViolation = TimingViolation::SLOT_EXEC_OVERSHOOT, ._slotIndex = _slotIndex, ._taskExecutionTime = executionTime};
      insert_violation(violationInfo);
    }
  }

  // second detect period timing violations
  SlotTimingInfo slotTimingInfo = get_slot_timing(_slotIndex);

  if (stopTime > slotTimingInfo.endTime + kPeriodOffsetTolerance) {
    if (has_violation(_slotIndex)) {
      update_violation_timing_violation(_slotIndex, TimingViolation::SLOT_BOUNDARY_CROSS);
      update_violation_end_time(_slotIndex, stopTime);
    } else {
      ViolationInfo violationInfo{
          ._timingViolation = TimingViolation::SLOT_BOUNDARY_CROSS, ._slotIndex = _slotIndex, ._taskEndTime = stopTime};
      insert_violation(violationInfo);
    }
  }

  // update slot index
  _slotIndex++;

  // update _stopCalled
  _stopCalled = true;
}

uint32_t TaskRecorder::get_nbr_of_periods() const {
  // Returns the current slot index. When _stopCalled is true, _slotIndex
  // was already incremented, so this represents the total number of periods processed.
  return _slotIndex;
}

uint32_t TaskRecorder::get_nbr_of_timing_violations() const {
  return _violationCount;
}

uint32_t TaskRecorder::get_nbr_of_period_timing_violations() const {
  return count_violations_by_flag(SLOT_PERIOD_VIOLATION);
}

uint32_t TaskRecorder::get_nbr_of_execution_overshoots() const {
  return count_violations_by_flag(SLOT_EXEC_OVERSHOOT);
}

const TaskRecorder::ViolationInfo* TaskRecorder::get_violation_info(uint16_t violationIndex) const {
  if (violationIndex >= _violationCount) {
    __ASSERT(false, "Violation index out of bounds");
    return nullptr;
  }
  return &_violations[violationIndex];
}

void TaskRecorder::print_all_violations() const {
  for (uint32_t i = 0; i < _violationCount; i++) {
    const ViolationInfo& violation = _violations[i];
    printk("Timing violation %d detected at slot index %d (task %s)\n",
           violation._timingViolation,
           violation._slotIndex,
           _taskInfo._szTaskName);
    switch (violation._timingViolation) {
    case car_system::TaskRecorder::TimingViolation::SLOT_EXEC_OVERSHOOT: {
      auto expectedComputationTime = get_expected_computation_time();
      printk("Expected task execution time: %lld usecs, got %lld usecs\n",
             expectedComputationTime.count(),
             violation._taskExecutionTime.count());
    } break;

    case car_system::TaskRecorder::TimingViolation::SLOT_MISSING: {
      printk("Slot %d is missing\n", violation._slotIndex);
    } break;

    case car_system::TaskRecorder::TimingViolation::SLOT_MULTIPLE_FIRE: {
      printk("Task was scheduled more than once in slot %d\n", violation._slotIndex);
    } break;

    case car_system::TaskRecorder::TimingViolation::SLOT_BOUNDARY_CROSS: {
      printk("Task was scheduled over multiple slots starting in slot %d\n", violation._slotIndex);
    } break;

    case car_system::TaskRecorder::TimingViolation::SLOT_PERIOD_VIOLATION: {
      printk("Multiple timing violations detected in slot %d\n", violation._slotIndex);
    } break;

    default: {
      printk("Unhandled violation: %d\n", violation._timingViolation);
    } break;
    }
  }
}

std::chrono::microseconds TaskRecorder::get_expected_computation_time() const {
#if CONFIG_PRIORITY_INVERSION
  using std::literals::chrono_literals::operator""us;
  std::chrono::microseconds computationTime = 0us;
  for (uint8_t subtaskIndex = 0; subtaskIndex < NbrOfSubTasks; subtaskIndex++) {
    computationTime += std::chrono::duration_cast<std::chrono::microseconds>(_taskInfo._subTasks[subtaskIndex]._computationTime);
  }
#else   // CONFIG_PRIORITY_INVERSION
  std::chrono::microseconds computationTime = std::chrono::duration_cast<std::chrono::microseconds>(_taskInfo._computationTime);
#endif  // CONFIG_PRIORITY_INVERSION
  return computationTime;
}

bool TaskRecorder::has_violation(uint32_t slot_index) const {
  return find_violation_by_slot(slot_index) != nullptr;
}

void TaskRecorder::insert_violation(const ViolationInfo& violationInfo) {
  if (_violationCount >= kMaxNbrOfViolations) {
    print_all_violations();
    zassert_true(_violationCount < kMaxNbrOfViolations, "Too many violations detected");
  }
  zassert_false(has_violation(violationInfo._slotIndex), "Violation already existing at index %d", violationInfo._slotIndex);
  _violations[_violationCount++] = violationInfo;
}

void TaskRecorder::update_violation_timing_violation(uint32_t slotIndex, TimingViolation timingViolation) {
  zassert_true(has_violation(slotIndex), "Violation not existing at index %d", slotIndex);
  ViolationInfo* pViolationInfo    = find_violation_by_slot(slotIndex);
  pViolationInfo->_timingViolation = static_cast<TimingViolation>(pViolationInfo->_timingViolation | timingViolation);
}

void TaskRecorder::update_violation_end_time(uint32_t slotIndex, const std::chrono::microseconds& endTime) {
  zassert_true(has_violation(slotIndex), "Violation not existing at index %d", slotIndex);
  ViolationInfo* const pViolationInfo = find_violation_by_slot(slotIndex);
  pViolationInfo->_taskEndTime        = endTime;
}

void TaskRecorder::update_violation_execution_time(uint32_t slotIndex, const std::chrono::microseconds& executionTime) {
  zassert_true(has_violation(slotIndex), "Violation not existing at index %d", slotIndex);
  ViolationInfo* const pViolationInfo = find_violation_by_slot(slotIndex);
  pViolationInfo->_taskExecutionTime  = executionTime;
}

}  // namespace car_system

#endif  // CONFIG_TEST == 1
