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
 * @brief TaskRecorder class declaration
 *
 * @date 2025-07-01
 * @version 1.0.0
 ***************************************************************************/

#pragma once

#if CONFIG_TEST
// zephyr
#include <zephyr/kernel.h>

// std
#include <chrono>
#include <map>

// local
#include "periodic_task_info.hpp"

// zpp_lib
#include "zpp_include/non_copyable.hpp"

namespace car_system {

using std::literals::chrono_literals::operator""us;

class TaskRecorder : private zpp_lib::NonCopyable<TaskRecorder> {
public:
  explicit TaskRecorder(const PeriodicTaskInfo& taskInfo);

  // called by the last thread reaching the barrier for setting the zero time
  static void set_zero_time(const std::chrono::microseconds& zeroTime);

  // called at every task computation start/stop
  void start();
  void stop();

  // used in test programs for validating that the car system
  void validate_timing();

  // constants applied when detecting timing violations
  static constexpr std::chrono::microseconds kPeriodOffsetTolerance = std::chrono::microseconds(CONFIG_PERIOD_OFFSET_TOLERANCE);
  static constexpr std::chrono::microseconds kComputationTimeOffsetTolerance =
      std::chrono::microseconds(CONFIG_COMPUTATION_TIME_OFFSET_TOLERANCE);

  // data structures and methods used for validating task timing
  // The constants defined above are applied for detection
  enum TimingViolation : uint8_t {
    OK                    = 0x00,
    SLOT_MISSING          = 0x01,                                                     // slot had no execution
    SLOT_MULTIPLE_FIRE    = 0x02,                                                     // slot had more than one execution
    SLOT_BOUNDARY_CROSS   = 0x04,                                                     // task stop crossed slot end
    SLOT_PERIOD_VIOLATION = SLOT_MISSING | SLOT_MULTIPLE_FIRE | SLOT_BOUNDARY_CROSS,  // all possible period timing violation
    SLOT_EXEC_OVERSHOOT   = 0x08,                                                     // execution time exceeded the task computation time
  };
  struct ViolationInfo {
    TimingViolation _timingViolation;
    uint32_t _slotIndex;
    std::chrono::microseconds _taskStartTime;
    std::chrono::microseconds _taskEndTime;
    std::chrono::microseconds _taskExecutionTime;
  };
  uint32_t get_nbr_of_periods() const;
  uint32_t get_nbr_of_timing_violations() const;
  uint32_t get_nbr_of_period_timing_violations() const;
  uint32_t get_nbr_of_execution_overshoots() const;
  const ViolationInfo* get_violation_info(uint16_t violationIndex) const;
  void print_all_violations() const;

private:
  // private methods
  std::chrono::microseconds get_expected_computation_time() const;
  bool has_violation(uint32_t slotIndex) const;
  void insert_violation(const ViolationInfo& violationInfo);
  void update_violation_timing_violation(uint32_t slotIndex, TimingViolation timingViolation);
  void update_violation_end_time(uint32_t slotIndex, const std::chrono::microseconds& endTime);
  void update_violation_execution_time(uint32_t slotIndex, const std::chrono::microseconds& executionTime);
  ViolationInfo* const find_violation_by_slot(uint32_t slot_index) {
    for (uint32_t i = 0; i < _violationCount; i++) {
      if (_violations[i]._slotIndex == slot_index) {
        return &_violations[i];
      }
    }
    return nullptr;
  }
  const ViolationInfo* const find_violation_by_slot(uint32_t slot_index) const {
    for (uint32_t i = 0; i < _violationCount; i++) {
      if (_violations[i]._slotIndex == slot_index) {
        return &_violations[i];
      }
    }
    return nullptr;
  }
  uint32_t count_violations_by_flag(uint32_t flag) const {
    uint32_t count = 0;
    for (uint32_t i = 0; i < _violationCount; i++) {
      if (_violations[i]._timingViolation & flag) {
        count++;
      }
    }
    return count;
  }

  struct SlotTimingInfo {
    std::chrono::microseconds startTime;
    std::chrono::microseconds endTime;
  };
  SlotTimingInfo get_slot_timing(uint32_t slotIndex) const {
    return {.startTime = slotIndex * _taskInfo._period, .endTime = (slotIndex + 1) * _taskInfo._period};
  }

  std::chrono::microseconds compute_execution_time() const {
#if CONFIG_USERSPACE
    return get_expected_computation_time();
#else   // CONFIG_USERSPACE
    struct k_thread_runtime_stats stats = {};
    k_thread_runtime_stats_get(k_current_get(), &stats);
    return std::chrono::microseconds(k_cyc_to_us_near64(stats.execution_cycles - _executionStartCycles));
#endif  // CONFIG_USERSPACE
  }

  // data members
  static std::chrono::microseconds _zeroTime;
  const PeriodicTaskInfo _taskInfo;
  uint32_t _slotIndex{0};
#if !CONFIG_USERSPACE
  uint64_t _executionStartCycles = {0};
#endif  // ! CONFIG_USERSPACE
  bool _stopCalled = true;
  std::chrono::microseconds _lastStartTime;
  static constexpr uint16_t kMaxNbrOfViolations  = 40;
  ViolationInfo _violations[kMaxNbrOfViolations] = {};
  uint16_t _violationCount                       = 0;
};

}  // namespace car_system

#endif  // CONFIG_TEST
