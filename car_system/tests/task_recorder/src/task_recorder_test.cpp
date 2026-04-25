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
 * @file task_recorder_test.cpp
 * @author Serge Ayer <serge.ayer@hefr.ch>
 *
 * @brief Test program for validating timing of the TaskRecorder implementation
 *
 * @date 2025-07-01
 * @version 1.0.0
 ***************************************************************************/
/* Scenarios:
 *   1. nominal          — task fires exactly once per slot, within limits
 *   2. exec_overshoot   — task deliberately runs longer than computationTime
 *   3. boundary_cross   — task starts near slot end, stop crosses boundary
 *   4. missing_slot     — task deliberately skips one activation
 *   5. multiple_fire    — task fires multiple times in one slot
 *   6. mixed            — mix errors
 */

// zephyr
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/ztest.h>

// stl
#include <chrono>

// zpp_lib
#include "zpp_include/barrier.hpp"
#include "zpp_include/this_thread.hpp"
#include "zpp_include/thread.hpp"
#include "zpp_include/time.hpp"

// local
#include "periodic_task_info.hpp"
#include "task_recorder.hpp"

LOG_MODULE_REGISTER(task_recorder_test, LOG_LEVEL_DBG);

// ── Task definitions ──────────────────────────────────────────────────────────
using std::literals::chrono_literals::operator""us;
using std::literals::chrono_literals::operator""ms;
using car_system::PeriodicTaskInfo;
using car_system::TaskRecorder;

// ── Scenario 1 — One task, no timing violation ────────────────────────────────
ZTEST(task_recorder_suite, test_one_task_no_violation) {
  // A 10ms periodic task with 3ms max computation time
  static constexpr PeriodicTaskInfo kTask10ms{._computationTime = 3ms, ._period = 10ms, ._szTaskName = "test_10ms"};
  TaskRecorder taskRecorder(kTask10ms);

  zpp_lib::Barrier barrier{1};
  auto startTime = barrier.wait(&TaskRecorder::set_zero_time);

  // Run exactly kNbrOfPeriods activations
  static constexpr uint8_t kNbrOfPeriods = 100;
  for (uint8_t i = 0; i < kNbrOfPeriods; i++) {
    taskRecorder.start();
    zpp_lib::ThisThread::busy_wait(kTask10ms._computationTime);
    taskRecorder.stop();

    startTime += kTask10ms._period;
    zpp_lib::ThisThread::sleep_until(startTime);
  }

  // validate that the correct number of periods was detected
  const auto nbrOfPeriods = taskRecorder.get_nbr_of_periods();
  zassert_equal(nbrOfPeriods, kNbrOfPeriods, "Expected %d period measurements, got %u", kNbrOfPeriods, nbrOfPeriods);
  // validate that no violation occured
  const auto nbrOfTimingViolations                   = taskRecorder.get_nbr_of_timing_violations();
  static constexpr uint32_t kExpectedNbrOfViolations = 0;
  zassert_equal(nbrOfTimingViolations, kExpectedNbrOfViolations, "Expected no timing violation, got %u", nbrOfTimingViolations);
}

// ── Scenario 2 — Exec overshoot ───────────────────────────────────────────────
// execution overshoots are not detected when user mode is enabled
#if !CONFIG_USERSPACE
ZTEST(task_recorder_suite, test_one_task_exec_overshoot) {
  // A 10ms periodic task with 3ms max computation time
  static constexpr PeriodicTaskInfo kTask10ms{._computationTime = 3ms, ._period = 10ms, ._szTaskName = "test_10ms"};
  TaskRecorder taskRecorder(kTask10ms);

  zpp_lib::Barrier barrier{1};
  auto startTime = barrier.wait(&TaskRecorder::set_zero_time);

  // Run exactly kNbrOfPeriods activations
  static constexpr uint8_t kNbrOfPeriods          = 100;
  static constexpr auto kOvershootComputationTime = kTask10ms._computationTime + 1ms;
  static constexpr auto kOvershootEveryNPeriod    = 5;
  for (uint8_t i = 0; i < kNbrOfPeriods; i++) {
    taskRecorder.start();
    if (i % kOvershootEveryNPeriod == 0) {
      zpp_lib::ThisThread::busy_wait(kOvershootComputationTime);
    } else {
      zpp_lib::ThisThread::busy_wait(kTask10ms._computationTime);
    }
    taskRecorder.stop();

    startTime += kTask10ms._period;
    zpp_lib::ThisThread::sleep_until(startTime);
  }

  // validate that the correct number of periods was detected
  const auto nbrOfPeriods = taskRecorder.get_nbr_of_periods();
  zassert_equal(nbrOfPeriods, kNbrOfPeriods, "Expected %d period measurements, got %u", kNbrOfPeriods, nbrOfPeriods);

  // validate that the number of timing violations was detected correctly (only execution
  // overshoots)
  static constexpr auto kExpectedNbrOfOvershoots = kNbrOfPeriods / kOvershootEveryNPeriod;
  const auto nbrOfTimingViolations               = taskRecorder.get_nbr_of_timing_violations();
  zassert_equal(nbrOfTimingViolations,
                kExpectedNbrOfOvershoots,
                "Expected %d timing violations, got %u",
                kExpectedNbrOfOvershoots,
                nbrOfTimingViolations);

  // validate that no period timing violation occured
  const auto nbrOfPeriodTimingViolation = taskRecorder.get_nbr_of_period_timing_violations();
  zassert_equal(nbrOfPeriodTimingViolation, 0, "Expected no period timing violation, got %u", nbrOfPeriodTimingViolation);

  // validate that execution overshoots were detected correctly
  const auto nbrOfExecutionOvershoots = taskRecorder.get_nbr_of_execution_overshoots();
  zassert_equal(nbrOfExecutionOvershoots,
                kExpectedNbrOfOvershoots,
                "Expected %d execution overshoots, got %u",
                kExpectedNbrOfOvershoots,
                nbrOfExecutionOvershoots);
  auto expectedPeriodEndTime = kTask10ms._period;
  for (uint32_t i = 0; i < nbrOfExecutionOvershoots; i++) {
    auto violation = taskRecorder.get_violation_info(i);
    zassert_true(violation != nullptr, "No violation with index %d", i);
    zassert_true(violation->_timingViolation == TaskRecorder::TimingViolation::SLOT_EXEC_OVERSHOOT,
                 "Violation should be SLOT_EXEC_OVERSHOOT, got %d",
                 violation->_timingViolation);
    auto expectedSlotEndTime = expectedPeriodEndTime + TaskRecorder::kPeriodOffsetTolerance;
    zassert_true(violation->_taskEndTime <= expectedSlotEndTime,
                 "Expected task end time is %lld msecs, got %lld msecs",
                 expectedSlotEndTime.count(),
                 violation->_taskEndTime.count());
    expectedPeriodEndTime += kOvershootEveryNPeriod * kTask10ms._period;
    std::chrono::microseconds expectedMaxExecutionTime = kOvershootComputationTime + TaskRecorder::kComputationTimeOffsetTolerance;
    zassert_true(violation->_taskExecutionTime <= expectedMaxExecutionTime,
                 "Task execution time should not exceed %lld usecs, got %lld usecs",
                 expectedMaxExecutionTime.count(),
                 violation->_taskExecutionTime.count());
    std::chrono::microseconds expectedMinExecutionTime = kOvershootComputationTime - TaskRecorder::kComputationTimeOffsetTolerance;
    zassert_true(violation->_taskExecutionTime >= expectedMinExecutionTime,
                 "Task execution time should be at least %lld usecs, got %lld usecs",
                 expectedMinExecutionTime.count(),
                 violation->_taskExecutionTime.count());
  }
}
#endif  // !CONFIG_USERSPACE

// ── Scenario 3 — Slot boundary crossing ───────────────────────────────────────────
ZTEST(task_recorder_suite, test_one_task_boundary_crossing) {
  // A 10ms periodic task with 3ms max computation time
  static constexpr PeriodicTaskInfo kTask10ms{._computationTime = 3ms, ._period = 10ms, ._szTaskName = "test_10ms"};
  TaskRecorder taskRecorder(kTask10ms);

  zpp_lib::Barrier barrier{1};
  auto startTime = barrier.wait(&TaskRecorder::set_zero_time);

  // Run exactly kNbrOfPeriods activations
  static constexpr uint8_t kNbrOfPeriods = 100;
  static constexpr auto kStartTimeForCrossing =
      std::chrono::duration_cast<std::chrono::microseconds>(kTask10ms._period - kTask10ms._computationTime + 1ms);
  static constexpr auto kSlotCrossingEveryNPeriod = 4;
  for (uint8_t i = 0; i < kNbrOfPeriods; i++) {
    taskRecorder.start();
    if (i % kSlotCrossingEveryNPeriod == 0) {
      zpp_lib::ThisThread::sleep_for(kStartTimeForCrossing);
    }
    zpp_lib::ThisThread::busy_wait(kTask10ms._computationTime);
    taskRecorder.stop();

    startTime += kTask10ms._period;
    zpp_lib::ThisThread::sleep_until(startTime);
  }

  // validate that the correct number of periods was detected
  const auto nbrOfPeriods = taskRecorder.get_nbr_of_periods();
  zassert_equal(nbrOfPeriods, kNbrOfPeriods, "Expected %d period measurements, got %u", kNbrOfPeriods, nbrOfPeriods);
  // validate that the number of timing violations was detected correctly (only execution
  // overshoots)
  static constexpr auto kExpectedNbrOfPeriodCrossings = kNbrOfPeriods / kSlotCrossingEveryNPeriod;
  const auto nbrOfTimingViolations                    = taskRecorder.get_nbr_of_timing_violations();
  zassert_equal(nbrOfTimingViolations,
                kExpectedNbrOfPeriodCrossings,
                "Expected %d timing violations, got %u",
                kExpectedNbrOfPeriodCrossings,
                nbrOfTimingViolations);
  // validate that no period timing violation occured
  const auto nbrOfPeriodTimingViolations = taskRecorder.get_nbr_of_period_timing_violations();
  zassert_equal(nbrOfPeriodTimingViolations,
                kExpectedNbrOfPeriodCrossings,
                "Expected %d period timing violation, got %u",
                kExpectedNbrOfPeriodCrossings,
                nbrOfPeriodTimingViolations);
  // validate that no execution overshoot was detected correctly
  const auto nbrOfExecutionOvershoots = taskRecorder.get_nbr_of_execution_overshoots();
  zassert_equal(nbrOfExecutionOvershoots, 0, "Expected no execution overshoot, got %u", nbrOfExecutionOvershoots);

  auto expectedPeriodStartTime = 0us;
  for (uint32_t i = 0; i < nbrOfPeriodTimingViolations; i++) {
    auto violation = taskRecorder.get_violation_info(i);
    zassert_true(violation != nullptr, "No violation with index %d", i);
    zassert_true(violation->_timingViolation == TaskRecorder::TimingViolation::SLOT_BOUNDARY_CROSS,
                 "Violation should be SLOT_BOUNDARY_CROSS, got %d",
                 violation->_timingViolation);
    auto expectedSlotStartTime = expectedPeriodStartTime + kStartTimeForCrossing + TaskRecorder::kPeriodOffsetTolerance;
    zassert_true(violation->_taskStartTime <= expectedSlotStartTime,
                 "Expected task start time is %lld usecs, got %lld usecs",
                 expectedSlotStartTime.count(),
                 violation->_taskStartTime.count());
    expectedPeriodStartTime += kSlotCrossingEveryNPeriod * kTask10ms._period;
  }
}

// ── Scenario 4 — Missing slot ─────────────────────────────────────────────────
ZTEST(task_recorder_suite, test_missing_slot) {
  // A 10ms periodic task with 3ms max computation time
  static constexpr PeriodicTaskInfo kTask10ms{._computationTime = 3ms, ._period = 10ms, ._szTaskName = "test_10ms"};
  TaskRecorder taskRecorder(kTask10ms);

  zpp_lib::Barrier barrier{1};
  auto startTime = barrier.wait(&TaskRecorder::set_zero_time);

  // Slot 0: normal activation
  taskRecorder.start();
  zpp_lib::ThisThread::busy_wait(kTask10ms._computationTime);
  taskRecorder.stop();
  startTime += kTask10ms._period;
  zpp_lib::ThisThread::sleep_until(startTime);

  // Slot 1: skip activation entirely — sleep through entire slot
  // This should trigger SLOT_MISSING for the skipped slot 1
  startTime += kTask10ms._period;
  zpp_lib::ThisThread::sleep_until(startTime);

  // Slot 2: normal activation
  taskRecorder.start();
  zpp_lib::ThisThread::busy_wait(kTask10ms._computationTime);
  taskRecorder.stop();
  startTime += kTask10ms._period;
  zpp_lib::ThisThread::sleep_until(startTime);

  // Slot 3: skip activation entirely — sleep through entire slot
  // This should trigger SLOT_MISSING for the skipped slot 3
  startTime += kTask10ms._period;
  zpp_lib::ThisThread::sleep_until(startTime);

  // Slot 4: skip activation entirely — sleep through entire slot
  // This should trigger SLOT_MISSING for the skipped slot 4
  startTime += kTask10ms._period;
  zpp_lib::ThisThread::sleep_until(startTime);

  // Slot 5: normal activation
  taskRecorder.start();
  zpp_lib::ThisThread::busy_wait(kTask10ms._computationTime);
  taskRecorder.stop();
  startTime += kTask10ms._period;
  zpp_lib::ThisThread::sleep_until(startTime);

  // validate that the correct number of periods was detected
  static constexpr uint32_t kExpectedNbrOfPeriods = 6;
  const auto nbrOfPeriods                         = taskRecorder.get_nbr_of_periods();
  zassert_equal(nbrOfPeriods, kExpectedNbrOfPeriods, "Expected %d period measurements, got %u", kExpectedNbrOfPeriods, nbrOfPeriods);
  // validate that violations were detected correctly
  static constexpr uint32_t kExpectedNbrOfViolations = 3;
  zassert_true(taskRecorder.get_nbr_of_timing_violations() == kExpectedNbrOfViolations,
               "Expected %d timing violations, got %d",
               kExpectedNbrOfViolations,
               taskRecorder.get_nbr_of_timing_violations());

  // Slot 1
  auto nextViolation = 0;
  auto violation     = taskRecorder.get_violation_info(nextViolation);
  zassert_true(violation != nullptr, "No violation with index %d", nextViolation);
  zassert_true(violation->_timingViolation == TaskRecorder::TimingViolation::SLOT_MISSING,
               "Violation should be SLOT_MISSING, got %d",
               violation->_timingViolation);
  uint32_t slotIndexViolation = 1;
  zassert_true(violation->_slotIndex == slotIndexViolation, "Missing slot should be %d, got %d", slotIndexViolation, violation->_slotIndex);

  // Slot 3
  nextViolation++;
  violation = taskRecorder.get_violation_info(nextViolation);
  zassert_true(violation != nullptr, "No violation with index %d", nextViolation);
  zassert_true(violation->_timingViolation == TaskRecorder::TimingViolation::SLOT_MISSING,
               "Violation should be SLOT_MISSING, got %d",
               violation->_timingViolation);
  slotIndexViolation = 3;
  zassert_true(violation->_slotIndex == slotIndexViolation, "Missing slot should be %d, got %d", slotIndexViolation, violation->_slotIndex);

  // Slot 4
  nextViolation++;
  violation = taskRecorder.get_violation_info(nextViolation);
  zassert_true(violation != nullptr, "No violation with index %d", nextViolation);
  zassert_true(violation->_timingViolation == TaskRecorder::TimingViolation::SLOT_MISSING,
               "Violation should be SLOT_MISSING, got %d",
               violation->_timingViolation);
  slotIndexViolation = 4;
  zassert_true(violation->_slotIndex == slotIndexViolation, "Missing slot should be %d, got %d", slotIndexViolation, violation->_slotIndex);
}

// ── Scenario 5 — Multiple fire ──────────────────────────────────────────────────
ZTEST(task_recorder_suite, test_multiple_fire) {
  // A 10ms periodic task with 3ms max computation time
  static constexpr PeriodicTaskInfo kTask10ms{._computationTime = 3ms, ._period = 10ms, ._szTaskName = "test_10ms"};
  TaskRecorder taskRecorder(kTask10ms);

  zpp_lib::Barrier barrier{1};
  auto startTime = barrier.wait(&TaskRecorder::set_zero_time);

  // Slot 0: double fire
  // Slot 0: normal first activation
  taskRecorder.start();
  zpp_lib::ThisThread::busy_wait(kTask10ms._computationTime);
  // stop the first task
  taskRecorder.stop();
  // Slot 0: second activation without sleeping till the end of period — still within slot
  // 0 This should trigger SLOT_MULTIPLE_FIRE for the double fire in slot 0 This should
  // also trigger SLOT_BOUNDARY_CROSS for slot 0
  auto nextFireTime = startTime + kTask10ms._period - kTask10ms._computationTime / 2;
  zpp_lib::ThisThread::sleep_until(nextFireTime);
  taskRecorder.start();
  zpp_lib::ThisThread::busy_wait(kTask10ms._computationTime);
  taskRecorder.stop();
  // sleep until period end (should not sleep)
  startTime += kTask10ms._period;
  zpp_lib::ThisThread::sleep_until(startTime);

  // Slot 1: normal activation — task starts in slot 1
  taskRecorder.start();
  zpp_lib::ThisThread::busy_wait(kTask10ms._computationTime);
  taskRecorder.stop();
  startTime += kTask10ms._period;
  zpp_lib::ThisThread::sleep_until(startTime);

  // Slot 2: multiple fire
  // Slot 2: normal first activation
  taskRecorder.start();
  zpp_lib::ThisThread::busy_wait(kTask10ms._computationTime);
  // stop the first task
  taskRecorder.stop();
  // Slot 2: second activation without waiting further — still within slot 2
  // This should trigger SLOT_MULTIPLE_FIRE for the second fire in slot 2
  taskRecorder.start();
  zpp_lib::ThisThread::busy_wait(kTask10ms._computationTime);
  taskRecorder.stop();
  // Slot 2: third activation without waiting further — still within slot 2
  // This should trigger SLOT_MULTIPLE_FIRE for the third fire in slot 2
  taskRecorder.start();
  zpp_lib::ThisThread::busy_wait(kTask10ms._computationTime);
  taskRecorder.stop();
  // sleep until period end
  startTime += kTask10ms._period;
  zpp_lib::ThisThread::sleep_until(startTime);

  // Slot 3: double fire with exec overshoot
  // Slot 3: normal first activation
  taskRecorder.start();
  zpp_lib::ThisThread::busy_wait(kTask10ms._computationTime);
  // stop the first task
  taskRecorder.stop();
  // Slot 3: second activation without sleeping till the end of period — still within slot
  // 3 This should trigger SLOT_MULTIPLE_FIRE for the double fire in slot 3
  zpp_lib::ThisThread::sleep_until(startTime + kTask10ms._period - kTask10ms._computationTime);
  taskRecorder.start();
  zpp_lib::ThisThread::busy_wait(kTask10ms._computationTime);
  // execution overshoot
  static constexpr auto kOvershootComputationTime{1ms};
  zpp_lib::ThisThread::busy_wait(kOvershootComputationTime);
  taskRecorder.stop();
  // sleep until period end (should not sleep)
  startTime += kTask10ms._period;
  zpp_lib::ThisThread::sleep_until(startTime);

  // validate that the correct number of periods was detected
  // there are 4 periods here but the last stop accounted for the next one
  static constexpr uint32_t kExpectedNbrOfPeriods = 4;
  const auto nbrOfPeriods                         = taskRecorder.get_nbr_of_periods();
  zassert_equal(nbrOfPeriods, kExpectedNbrOfPeriods, "Expected %d period measurements, got %u", kExpectedNbrOfPeriods, nbrOfPeriods);
  // validate that violations were detected correctly
  static constexpr uint32_t kExpectedNbrOfViolations = 3;
  zassert_true(taskRecorder.get_nbr_of_timing_violations() == kExpectedNbrOfViolations,
               "Expected %d timing violations, got %d",
               kExpectedNbrOfViolations,
               taskRecorder.get_nbr_of_timing_violations());

  // Slot 0
  auto nextViolation = 0;
  auto violation     = taskRecorder.get_violation_info(nextViolation);
  zassert_true(violation != nullptr, "No violation with index %d", nextViolation);
  zassert_true(violation->_timingViolation ==
                   (TaskRecorder::TimingViolation::SLOT_MULTIPLE_FIRE | TaskRecorder::TimingViolation::SLOT_BOUNDARY_CROSS),
               "Violation should be (SLOT_MULTIPLE_FIRE | SLOT_BOUNDARY_CROSS), got %d",
               violation->_timingViolation);
  uint32_t slotIndexViolation = 0;
  zassert_true(violation->_slotIndex == slotIndexViolation,
               "Double fire slot index should be %d, got %d",
               slotIndexViolation,
               violation->_slotIndex);

  // Slot 2
  nextViolation++;
  violation = taskRecorder.get_violation_info(nextViolation);
  zassert_true(violation != nullptr, "No violation with index %d", nextViolation);
  zassert_true(violation->_timingViolation == TaskRecorder::TimingViolation::SLOT_MULTIPLE_FIRE,
               "Violation should be SLOT_MULTIPLE_FIRE, got %d",
               violation->_timingViolation);
  slotIndexViolation = 2;
  zassert_true(violation->_slotIndex == slotIndexViolation,
               "Multiple fire / exec overshoot slot should be %d, got %d",
               slotIndexViolation,
               violation->_slotIndex);

  // Slot 3
  nextViolation++;
  violation = taskRecorder.get_violation_info(nextViolation);
  zassert_true(violation != nullptr, "No violation with index %d", nextViolation);
  // execution overshoots are not detected when user mode is enabled
#if CONFIG_USERSPACE
  static constexpr auto expectedViolation =
      (TaskRecorder::TimingViolation::SLOT_MULTIPLE_FIRE | TaskRecorder::TimingViolation::SLOT_BOUNDARY_CROSS);
#else   // CONFIG_USERSPACE
  static constexpr auto expectedViolation =
      (TaskRecorder::TimingViolation::SLOT_MULTIPLE_FIRE | TaskRecorder::TimingViolation::SLOT_BOUNDARY_CROSS |
       TaskRecorder::TimingViolation::SLOT_EXEC_OVERSHOOT);
#endif  // CONFIG_USERSPACE
  zassert_true(
      violation->_timingViolation == expectedViolation, "Violation should be %d, got %d", expectedViolation, violation->_timingViolation);
  slotIndexViolation = 3;
  zassert_true(violation->_slotIndex == slotIndexViolation,
               "Double fire / exec overshoot slot should be %d, got %d",
               slotIndexViolation,
               violation->_slotIndex);
  // execution overshoots are not detected when user mode is enabled
#if !CONFIG_USERSPACE
  auto expectedTaskExecutionTime =
      std::chrono::duration_cast<std::chrono::microseconds>(kTask10ms._computationTime + kOvershootComputationTime);
  zassert_within(violation->_taskExecutionTime,
                 expectedTaskExecutionTime,
                 TaskRecorder::kComputationTimeOffsetTolerance,
                 "Expected task execution time is %lld usecs, got %lld usecs",
                 expectedTaskExecutionTime.count(),
                 violation->_taskExecutionTime.count());
#endif  // !CONFIG_USERSPACE
}

// ── Scenario 6 — Mixing different errors
// ──────────────────────────────────────────────────
ZTEST(task_recorder_suite, test_mixed_errors) {
  // A 10ms periodic task with 3ms max computation time
  static constexpr PeriodicTaskInfo kTask10ms{._computationTime = 3ms, ._period = 10ms, ._szTaskName = "test_10ms"};
  TaskRecorder taskRecorder(kTask10ms);

  zpp_lib::Barrier barrier{1};
  auto startTime = barrier.wait(&TaskRecorder::set_zero_time);

  // Slot 0: normal activation
  taskRecorder.start();
  zpp_lib::ThisThread::busy_wait(kTask10ms._computationTime);
  taskRecorder.stop();
  startTime += kTask10ms._period;
  zpp_lib::ThisThread::sleep_until(startTime);

  // Slot 1: late activation
  auto lateness = kTask10ms._period - kTask10ms._computationTime / 2;
  zpp_lib::ThisThread::sleep_until(startTime + lateness);
  taskRecorder.start();
  zpp_lib::ThisThread::busy_wait(kTask10ms._computationTime);
  taskRecorder.stop();
  startTime += kTask10ms._period;
  zpp_lib::ThisThread::sleep_until(startTime);

  // Slot 2: skip activation entirely — sleep through entire slot
  startTime += kTask10ms._period;
  zpp_lib::ThisThread::sleep_until(startTime);

  // Slot 3: normal activation, with execution overshoot
  taskRecorder.start();
  zpp_lib::ThisThread::busy_wait(kTask10ms._computationTime);
  static constexpr auto kOvershootComputationTime{1ms};
  zpp_lib::ThisThread::busy_wait(kOvershootComputationTime);
  taskRecorder.stop();
  startTime += kTask10ms._period;
  zpp_lib::ThisThread::sleep_until(startTime);

  // Slot 4: late activation, with execution overshoot
  zpp_lib::ThisThread::sleep_until(startTime + kTask10ms._period - kTask10ms._computationTime / 2);
  taskRecorder.start();
  zpp_lib::ThisThread::busy_wait(kTask10ms._computationTime);
  zpp_lib::ThisThread::busy_wait(kOvershootComputationTime);
  taskRecorder.stop();
  startTime += kTask10ms._period;
  zpp_lib::ThisThread::sleep_until(startTime);

  // validate that the correct number of periods was detected
  static constexpr uint32_t kExpectedNbrOfPeriods = 5;
  const auto nbrOfPeriods                         = taskRecorder.get_nbr_of_periods();
  zassert_equal(nbrOfPeriods, kExpectedNbrOfPeriods, "Expected %d period measurements, got %u", kExpectedNbrOfPeriods, nbrOfPeriods);
  // validate that violations were detected correctly
  // execution overshoots are not detected when user mode is enabled
#if CONFIG_USERSPACE
  static constexpr uint32_t kExpectedNbrOfViolations = 3;
#else   // CONFIG_USERSPACE
  static constexpr uint32_t kExpectedNbrOfViolations = 4;
#endif  // CONFIG_USERSPACE
  zassert_true(taskRecorder.get_nbr_of_timing_violations() == kExpectedNbrOfViolations,
               "Expected %d timing violations, got %d",
               kExpectedNbrOfViolations,
               taskRecorder.get_nbr_of_timing_violations());

  // Slot 1
  auto nextViolation = 0;
  auto violation     = taskRecorder.get_violation_info(nextViolation);
  zassert_true(violation != nullptr, "No violation with index %d", nextViolation);
  zassert_true(violation->_timingViolation == TaskRecorder::TimingViolation::SLOT_BOUNDARY_CROSS,
               "Violation should be SLOT_BOUNDARY_CROSS, got %d",
               violation->_timingViolation);
  uint32_t slotIndexViolation = 1;
  zassert_true(violation->_slotIndex == slotIndexViolation,
               "SLOT_BOUNDARY_CROSS fire slot index should be %d, got %d",
               slotIndexViolation,
               violation->_slotIndex);

  // Slot 2
  nextViolation++;
  violation = taskRecorder.get_violation_info(nextViolation);
  zassert_true(violation != nullptr, "No violation with index %d", nextViolation);
  zassert_true(violation->_timingViolation == TaskRecorder::TimingViolation::SLOT_MISSING,
               "Violation should be SLOT_MISSING, got %d",
               violation->_timingViolation);
  slotIndexViolation = 2;
  zassert_true(violation->_slotIndex == slotIndexViolation, "Missing slot should be %d, got %d", slotIndexViolation, violation->_slotIndex);

  // Slot 3
  // execution overshoots are not detected when user mode is enabled
#if !CONFIG_USERSPACE
  nextViolation++;
  violation = taskRecorder.get_violation_info(nextViolation);
  zassert_true(violation != nullptr, "No violation with index %d", nextViolation);
  zassert_true(violation->_timingViolation == TaskRecorder::TimingViolation::SLOT_EXEC_OVERSHOOT,
               "Violation should be SLOT_EXEC_OVERSHOOT, got %d",
               violation->_timingViolation);
  slotIndexViolation = 3;
  zassert_true(violation->_slotIndex == slotIndexViolation,
               "Execution overshoot slot should be %d, got %d",
               slotIndexViolation,
               violation->_slotIndex);
#endif  // !CONFIG_USERSPACE

  // Slot 4
  nextViolation++;
  violation = taskRecorder.get_violation_info(nextViolation);
  zassert_true(violation != nullptr, "No violation with index %d", nextViolation);
#if CONFIG_USERSPACE
  static constexpr auto expectedViolation = TaskRecorder::TimingViolation::SLOT_BOUNDARY_CROSS;
#else   // CONFIG_USERSPACE
  static constexpr auto expectedViolation =
      (TaskRecorder::TimingViolation::SLOT_BOUNDARY_CROSS | TaskRecorder::TimingViolation::SLOT_EXEC_OVERSHOOT);
#endif  // CONFIG_USERSPACE
  zassert_true(
      violation->_timingViolation == expectedViolation, "Violation should be %d, got %d", expectedViolation, violation->_timingViolation);
  slotIndexViolation = 4;
  zassert_true(violation->_slotIndex == slotIndexViolation,
               "Boundary cross / Execution overshoot slot should be %d, got %d",
               slotIndexViolation,
               violation->_slotIndex);
#if !CONFIG_USERSPACE
  auto expectedTaskExecutionTime =
      std::chrono::duration_cast<std::chrono::microseconds>(kTask10ms._computationTime + kOvershootComputationTime);
  zassert_within(violation->_taskExecutionTime,
                 expectedTaskExecutionTime,
                 TaskRecorder::kComputationTimeOffsetTolerance,
                 "Expected task execution time is %lld usecs, got %lld usecs",
                 expectedTaskExecutionTime.count(),
                 violation->_taskExecutionTime.count());
#endif  // !CONFIG_USERSPACE
}

ZTEST_SUITE(task_recorder_suite, nullptr, nullptr, nullptr, nullptr, nullptr);
