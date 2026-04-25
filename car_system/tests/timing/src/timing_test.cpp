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
 * @file timing_test.cpp
 * @author Serge Ayer <serge.ayer@hefr.ch>
 *
 * @brief Test program for validating timing of the CarSystem implementation
 *
 * @date 2025-07-01
 * @version 1.0.0
 ***************************************************************************/

// zephyr
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/ztest.h>

// zpp_lib
#include "zpp_include/this_thread.hpp"

// local
#include "car_system.hpp"
#if CONFIG_USERSPACE
#include "userspace/init_domain.hpp"
#endif  // CONFIG_USERSPACE

LOG_MODULE_REGISTER(car_system, CONFIG_APP_LOG_LEVEL);

// ── Construction tests ────────────────────────────────────────────────────────

// for ms or s literals
using std::literals::chrono_literals::operator""s;

// Different modes
// nrf5340, busy: 22s
// qemu_x86, busy: tested up to 120s
static constexpr std::chrono::milliseconds testDuration = 60s;

#if CONFIG_USERSPACE
// create the CarSystem instance
APP_DATA static car_system::CarSystem carSystem;
#endif  // CONFIG_USERSPACE

ZTEST(car_system_suite, test_timing) {
#if !CONFIG_USERSPACE
  // create the CarSystem instance
  car_system::CarSystem carSystem;
#endif  // ! CONFIG_USERSPACE

#if CONFIG_USERSPACE
  car_system::init_domain();
#endif  // CONFIG_USERSPACE

#if CONFIG_USERSPACE
  // run the CarSystem in a separate thread (highest priority for executing start without
  // preemption)
  zpp_lib::Thread thread(zpp_lib::PreemptableThreadPriority::PriorityRealtime, "Test CarSystem");
  auto res = thread.start(std::bind(&car_system::CarSystem::start, &carSystem));
  zassert_true(res, "Could not start thread");
#else   // CONFIG_USERSPACE
  zpp_lib::Thread thread(zpp_lib::PreemptableThreadPriority::PriorityNormal, "Test CarSystem");
  LOG_DBG("Starting thread");
  auto res = thread.start(std::bind(&car_system::CarSystem::start, &carSystem));
  zassert_true(res, "Could not start thread");
#endif  // CONFIG_USERSPACE

  // let the bike system run for the test duration
  zpp_lib::ThisThread::sleep_for(testDuration);

  // stop the bike system
  carSystem.stop();

  // wait for thread to terminate
  res = thread.join();
  zassert_true(res, "Could not join thread");

  // validate timing constraints
  for (auto taskIndex = 0; taskIndex < car_system::CarSystem::kNbrOfPeriodicTasks; taskIndex++) {
    car_system::TaskRecorder& taskRecorder = carSystem.get_task_recorder(taskIndex);
    const auto nbrOfTimingViolations       = taskRecorder.get_nbr_of_timing_violations();
    if (nbrOfTimingViolations > 0) {
      taskRecorder.print_all_violations();
      zassert_equal(nbrOfTimingViolations, 0, "Expected no timing violation for task %d, got %u", taskIndex, nbrOfTimingViolations);
    }
  }
}

ZTEST_SUITE(car_system_suite, nullptr, nullptr, nullptr, nullptr, nullptr);
