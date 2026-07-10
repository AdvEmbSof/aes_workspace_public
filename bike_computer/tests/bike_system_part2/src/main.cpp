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
 * @brief Test program for the BikeSystem class (codelab part 1)
 *
 * @date 2025-07-01
 * @version 1.0.0
 ***************************************************************************/

// zephyr

// std
#include <chrono>
#include <cstdio>

// zpp_lib
#include "zpp_include/this_thread.hpp"
#include "zpp_include/thread.hpp"
#include "zpp_include/zpp_assert.hpp"
#include "zpp_include/zpp_log.hpp"
#include "zpp_include/zpp_test.hpp"

// bike computer
#include "static_scheduling_with_event/bike_system.hpp"

ZPP_LOG_MODULE_REGISTER(bike_computer, CONFIG_APP_LOG_LEVEL);

// for ms or s literals
using std::literals::chrono_literals::operator""s;

// TODO(Student): Validate the test duration for your target platform.
// The value of TEST_DURATION_IN_MS is set in the test case configuration file (testcase.yaml) and
// can be overridden for each platform. The value should be set so that the test passes on your platform.
// It should not be below 20s for any platform.
static constexpr std::chrono::milliseconds kTestDuration(CONFIG_TEST_DURATION_IN_MS);

// test_bike_system_static handler function
ZPP_ZTEST(bike_system_part2, test_bike_system_static_with_event) {
  // create the BikeSystem instance
  bike_computer::static_scheduling_with_event::BikeSystem bike_system;

  // run the bike system in a separate thread
  zpp_lib::Thread thread(zpp_lib::PreemptableThreadPriority::PriorityNormal, "Test BS static with event");
  ZPP_LOG_DBG("Starting thread");
  auto res = thread.start([&bike_system]() {
    auto res = bike_system.start();
    zpp_zassert_true(res, "BikeSystem start failed");
  });
  zpp_zassert_true(res, "Could not start thread");

  // let the bike system run for the test duration
  zpp_lib::ThisThread::sleep_for(kTestDuration);

  // stop the bike system
  bike_system.stop();

  // wait for thread to terminate
  res = thread.join();
  zpp_zassert_true(res, "Could not join thread");
}

ZPP_ZTEST_SUITE(bike_system_part2, nullptr, nullptr, nullptr, nullptr, nullptr);
