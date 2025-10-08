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
 * @file ttce.hpp
 * @author Serge Ayer <serge.ayer@hefr.ch>
 *
 * @brief TTCE implementation
 *
 * @date 2025-07-01
 * @version 1.0.0
 ***************************************************************************/

#pragma once

// zephyr
#include <zephyr/kernel.h>

// std
#include <chrono>
#include <string>

// zpp_lib
#include "zpp_include/clock.hpp"
#include "zpp_include/non_copyable.hpp"
#include "zpp_include/zephyr_result.hpp"

namespace bike_computer {

template <typename F, uint16_t NbrOfMinorCycles, uint16_t MaxMinorCycleSize>
class TTCE : private zpp_lib::NonCopyable<TTCE<F, NbrOfMinorCycles, MaxMinorCycleSize>> {
 public:
  explicit TTCE(std::chrono::milliseconds minorCycle) : _minorCycle(minorCycle) {
    k_timer_init(&_timer, &TTCE::_thunk, nullptr);
    // specify this instance as user data
    // this cast is ugly but the only way to pass a reference to this instance to the
    // timer
    // cppcheck-suppress cstyleCast
    _timer.user_data = (void*)this;  // NOLINT(readability/casting)
    k_work_init(&_work, &TTCE::_workHandler);
    // initialize the work queue
    k_work_queue_init(&_workQueue);
  }

  void start() {
    // first start the timer
    k_timeout_t period = zpp_lib::milliseconds_to_ticks(_minorCycle);
    k_timer_start(&_timer, K_SECONDS(0), period);

    // then run the work queue
    struct k_work_queue_config cfg = {
        .name     = "TTCE Work Queue",
        .no_yield = true,
    };
    _isStarted = true;
    k_work_queue_run(&_workQueue, &cfg);
  }

  void stop() {
    // first stop the time
    k_timer_stop(&_timer);
    // drain the work queue
    auto rc = k_work_queue_drain(&_workQueue, true);
    if (rc < 0) {
      __ASSERT(false, "k_work_queue_drain failed with code %d", rc);
    }
    rc = k_work_queue_stop(&_workQueue, K_SECONDS(1));
    if (rc != 0) {
      __ASSERT(false, "k_work_queue_stop failed with code %d", rc);
    }
  }

  bool isStarted() { return _isStarted; }

  [[nodiscard]] zpp_lib::ZephyrResult addTask(uint16_t minorCycleIndex, F f) {
    zpp_lib::ZephyrResult res;
    if (minorCycleIndex >= NbrOfMinorCycles) {
      __ASSERT(false, "Invalid minor cycle index %d", minorCycleIndex);
      res.assign_error(zpp_lib::ZephyrErrorCode::k_inval);
      return res;
    }
    if (_nbrOfTasksInMinorCycle[minorCycleIndex] >= MaxMinorCycleSize) {
      __ASSERT(false,
               "Too many tasks in minor cycle %d: %d",
               minorCycleIndex,
               _nbrOfTasksInMinorCycle[minorCycleIndex] + 1);
      res.assign_error(zpp_lib::ZephyrErrorCode::k_inval);
      return res;
    }

    _tasks[minorCycleIndex][_nbrOfTasksInMinorCycle[minorCycleIndex]] = f;
    _nbrOfTasksInMinorCycle[minorCycleIndex]++;

    return res;
  }

 private:
  static void _thunk(struct k_timer* timer_id) {
    // submit the periodic TTCE task
    if (timer_id != nullptr) {
      // get instance from user data
      // this cast is ugly but the only way to pass a reference to this instance to the
      // timer
      // cppcheck-suppress cstyleCast
      TTCE* pTTCE = (TTCE*)timer_id->user_data;  // NOLINT(readability/casting)
      auto ret    = k_work_submit_to_queue(&pTTCE->_workQueue, &pTTCE->_work);
      if (ret != 0 && ret != 1 && ret != 2) {
        __ASSERT(false, "Failed to submit work: %d", ret);
        return;
      }
    }
  }

  static void _workHandler(struct k_work* item) {
    // this ugly casting is the simplest way of getting the information
    // we need in the _workHandler method
    // CASTING IS POSSIBLE ONLY WHEN k_work IS THE FIRST ATTRIBUTE IN THE CLASS
    // cppcheck-suppress dangerousTypeCast
    TTCE* pTTCE = (TTCE*)item;  // NOLINT(readability/casting)

    // execute tasks based on schedule table
    for (uint16_t taskIndex = 0; taskIndex < MaxMinorCycleSize; taskIndex++) {
      if (pTTCE->_tasks[pTTCE->_minorCycleIndex][taskIndex] != nullptr) {
        pTTCE->_tasks[pTTCE->_minorCycleIndex][taskIndex]();
      }
    }
    pTTCE->_minorCycleIndex = (pTTCE->_minorCycleIndex + 1) % NbrOfMinorCycles;
  }

  // _work MUST be the first attribute
  struct k_work _work;
  struct k_work_q _workQueue;
  bool _isStarted = false;
  struct k_timer _timer;
  std::chrono::milliseconds _minorCycle;
  uint16_t _minorCycleIndex                          = 0;
  F _tasks[NbrOfMinorCycles][MaxMinorCycleSize]      = {nullptr};
  uint16_t _nbrOfTasksInMinorCycle[NbrOfMinorCycles] = {0};
};

}  // namespace bike_computer
