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
 * @file test_bike_system_part2.cpp
 * @author Serge Ayer <serge.ayer@hefr.ch>
 *
 * @brief Test program for the BikeSystem class (codelab part 2)
 *
 * @date 2025-07-01
 * @version 1.0.0
 ***************************************************************************/

// zephyr
#include <zephyr/logging/log.h>
#include <zephyr/ztest.h>

// std
#include <chrono>
#include <cstdio>

// zpp_lib
#include "zpp_include/this_thread.hpp"
#include "zpp_include/thread.hpp"

// bike computer
#include "static_scheduling_with_event/bike_system.hpp"

LOG_MODULE_REGISTER(bike_system, CONFIG_APP_LOG_LEVEL);

// for ms or s literals
using std::literals::chrono_literals::operator""ms;

// Different modes
// nrf5340, busy: 48s
// nrf5340, sleep: 48s
// qemu_x86, busy: tested up to 120s
// qemy_x86, sleep: tested up to 120s
static constexpr std::chrono::milliseconds testDuration = 48s;

// test_bike_system_event_queue handler function
ZTEST(bike_system_part2, test_bike_system_ttce) {
  // create the BikeSystem instance
  static bike_computer::static_scheduling_with_event::BikeSystem bikeSystem;

  // run the bike system in a separate thread
  zpp_lib::Thread thread(zpp_lib::PreemptableThreadPriority::PriorityNormal, "Test BS TTCE");
  auto res = thread.start(std::bind(&bike_computer::static_scheduling_with_event::BikeSystem::start, &bikeSystem));
  zassert_true(res, "Could not start thread");

  // let the bike system run for the test duration
  zpp_lib::ThisThread::sleep_for(testDuration);

  // stop the bike system
  bikeSystem.stop();

  res = thread.join();
  zassert_true(res, "Could not join thread");
}

ZTEST_SUITE(bike_system_part2, NULL, NULL, NULL, NULL, NULL);
