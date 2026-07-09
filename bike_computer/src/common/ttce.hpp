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
#include "zpp_include/zephyr_result.hpp"
#include "zpp_include/zpp_assert.hpp"

namespace bike_computer {

template <typename F, uint16_t NbrOfMinorCycles, uint16_t MaxMinorCycleSize> class TTCE {
public:
  explicit TTCE(std::chrono::milliseconds minorCycle) : _minor_cycle(minorCycle) {
    k_timer_init(&_timer, &TTCE::s_thunk, nullptr);
    // specify this instance as user data
    // this cast is ugly but the only way to pass a reference to this instance to the
    // timer
    // NOLINTNEXTLINE(modernize-avoid-c-style-cast)
    _timer.user_data = (void*)this;  // NOLINT(readability/casting)
    k_work_init(&_work, &TTCE::s_work_handler);
    // initialize the work queue
    k_work_queue_init(&_work_queue);
  }

  ~TTCE() {
    stop();
  }

  /** Explicity prevent (move) copy and assignment
      rather than inheriting from NonCopyable. This avoids
      cppcoreguidelines-special-member-functions warning by clang-tidy.
  */
  TTCE(const TTCE&)            = delete;
  TTCE(TTCE&&)                 = delete;
  TTCE& operator=(const TTCE&) = delete;
  TTCE& operator=(TTCE&&)      = delete;

  void start() {
    // first start the timer
    k_timeout_t period = zpp_lib::milliseconds_to_ticks(_minor_cycle);
    k_timer_start(&_timer, period, period);

    // then run the work queue
    struct k_work_queue_config cfg = {
        .name     = "TTCE Work Queue",
        .no_yield = true,
    };
    _is_started = true;
    k_work_queue_run(&_work_queue, &cfg);
  }

  // Complexity is increased by zephyr macros
  // NOLINTNEXTLINE(readability-function-cognitive-complexity)
  void stop() {
    if (!_is_started) {
      return;
    }
    // first stop the time
    k_timer_stop(&_timer);
    // drain the work queue
    auto ret = k_work_queue_drain(&_work_queue, true);
    if (ret < 0) {
      ZPP_ASSERT(false, "k_work_queue_drain failed with code %d", ret);
    }
    // This is a Zephyr macro
    // NOLINTNEXTLINE(readability-math-missing-parentheses)
    ret = k_work_queue_stop(&_work_queue, K_SECONDS(1));
    if (ret != 0) {
      ZPP_ASSERT(false, "k_work_queue_stop failed with code %d", ret);
    }
    _is_started = false;
  }

  bool is_started() {
    return _is_started;
  }

  void add_initial_task(F f) {
    _initial_task = std::move(f);
  }

  [[nodiscard]] zpp_lib::ZephyrResult add_task(uint16_t minor_cycle_index, F f) {
    zpp_lib::ZephyrResult res;
    if (minor_cycle_index >= NbrOfMinorCycles) {
      ZPP_ASSERT(false, "Invalid minor cycle index %d", minor_cycle_index);
      res.assign_error(zpp_lib::ZephyrErrorCode::Inval);
      return res;
    }
    // Everything is known at compile time, so we can safely use the minor_cycle_index as an index
    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-constant-array-index)
    if (_nbr_of_tasks_in_minor_cycle[minor_cycle_index] >= MaxMinorCycleSize) {
      ZPP_ASSERT(false, "Too many tasks in minor cycle %d: %d", minor_cycle_index, _nbr_of_tasks_in_minor_cycle[minor_cycle_index] + 1);
      res.assign_error(zpp_lib::ZephyrErrorCode::Inval);
      return res;
    }

    _tasks[minor_cycle_index][_nbr_of_tasks_in_minor_cycle[minor_cycle_index]] = std::move(f);
    _nbr_of_tasks_in_minor_cycle[minor_cycle_index]++;
    // NOLINTEND(cppcoreguidelines-pro-bounds-constant-array-index)

    return res;
  }

private:
  static void s_thunk(struct k_timer* timer_id) {
    // submit the periodic TTCE task
    if (timer_id != nullptr) {
      // get instance from user data
      // this cast is ugly but the only way to pass a reference
      // to this instance to the timer
      // NOLINTNEXTLINE(modernize-avoid-c-style-cast)
      TTCE* p_ttce = (TTCE*)timer_id->user_data;  // NOLINT(readability/casting)
      auto ret     = k_work_submit_to_queue(&p_ttce->_work_queue, &p_ttce->_work);
      if (ret != 0 && ret != 1 && ret != 2) {
        ZPP_ASSERT(false, "Failed to submit work: %d", ret);
        return;
      }
    }
  }

  static void s_work_handler(struct k_work* item) {
    // this ugly casting is the simplest way of getting the information
    // we need in the s_workHandler method
    // CASTING IS POSSIBLE ONLY WHEN k_work IS THE FIRST ATTRIBUTE IN THE CLASS
    // NOLINTNEXTLINE(modernize-avoid-c-style-cast)
    TTCE* p_ttce = (TTCE*)item;  // NOLINT(readability/casting)

    // if an initial task is set, execute it and reset it
    if (p_ttce->_initial_task != nullptr) {
      p_ttce->_initial_task();
      p_ttce->_initial_task = nullptr;
    }

    // execute tasks based on schedule table
    // Everything is known at compile time, so we can safely use the minor_cycle_index as an index
    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-constant-array-index)
    for (uint16_t task_index = 0; task_index < MaxMinorCycleSize; task_index++) {
      if (p_ttce->_tasks[p_ttce->_minor_cycle_index][task_index] != nullptr) {
        p_ttce->_tasks[p_ttce->_minor_cycle_index][task_index]();
      }
    }
    p_ttce->_minor_cycle_index = (p_ttce->_minor_cycle_index + 1) % NbrOfMinorCycles;
    // NOLINTEND(cppcoreguidelines-pro-bounds-constant-array-index)
  }

  // _work MUST be the first attribute
  struct k_work _work         = {};
  struct k_work_q _work_queue = {};
  bool _is_started            = false;
  struct k_timer _timer       = {};
  std::chrono::milliseconds _minor_cycle;
  uint16_t _minor_cycle_index = 0;
  // Declaring these arrays as static constexpr allows the compiler to optimize them and avoid unnecessary copies.
  // NOLINTBEGIN(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  F _tasks[NbrOfMinorCycles][MaxMinorCycleSize]           = {nullptr};
  uint16_t _nbr_of_tasks_in_minor_cycle[NbrOfMinorCycles] = {0};
  // NOLINTEND(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  F _initial_task = nullptr;
};

}  // namespace bike_computer
