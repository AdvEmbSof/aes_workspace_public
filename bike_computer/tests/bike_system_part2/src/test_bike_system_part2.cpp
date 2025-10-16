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
 * @file test_bike_system.cpp
 * @author Serge Ayer <serge.ayer@hefr.ch>
 *
 * @brief Test program for the BikeSystem class
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
using namespace std::literals;

static constexpr std::chrono::milliseconds testDuration = 10s;

// test_bike_system_event_queue handler function
ZTEST(bike_system, test_bike_system_ttce) {
  // create the BikeSystem instance
  static bike_computer::static_scheduling_with_event::BikeSystem bikeSystem;

  // run the bike system in a separate thread
  zpp_lib::Thread thread(zpp_lib::PreemptableThreadPriority::PriorityNormal,
                         "Test BS TTCE");
  auto res = thread.start(
      std::bind(&bike_computer::static_scheduling_with_event::BikeSystem::start, &bikeSystem));
  zassert_true(res, "Could not start thread");

  // let the bike system run for the test duration
  zpp_lib::ThisThread::sleep_for(testDuration);

  // stop the bike system
  bikeSystem.stop();

  // wait for thread to terminate
  zpp_lib::ThisThread::sleep_for(5s);
  
#ifdef CONFIG_BOARD_QEMU_X86
    printk("Skipping join on QEMU\n");
#else
    res = thread.join();
    zassert_true(res, "Could not join thread");
#endif
}

#if defined(ALL_TESTS)
// test_bike_system_with_event handler function
ZTEST(bike_system, test_bike_system_with_event) {
  // create the BikeSystem instance
  static_scheduling_with_event::BikeSystem bikeSystem;

  // run the bike system in a separate thread
  Thread thread;
  thread.start(callback(&bikeSystem, &static_scheduling_with_event::BikeSystem::start));

  // let the bike system run for 20 secs
  ThisThread::sleep_for(20s);

  // stop the bike system
  bikeSystem.stop();

  // check whether scheduling was correct
  // Order is kGearTaskIndex, kSpeedTaskIndex, kTemperatureTaskIndex,
  //          kResetTaskIndex, kDisplayTask1Index, kDisplayTask2Index
  // When we use event handling, we do not check the computation time
  constexpr std::chrono::microseconds taskPeriods[] = {
      800000us, 400000us, 1600000us, 800000us, 1600000us, 1600000us};

  // allow for 2 msecs offset (with EventQueue)
  constexpr uint64_t kDeltaUs = 2000;
  for (uint8_t taskIndex = 0; taskIndex < advembsof::TaskLogger::kNbrOfTasks;
       taskIndex++) {
    TEST_ASSERT_UINT64_WITHIN(kDeltaUs,
                              taskPeriods[taskIndex].count(),
                              bikeSystem.getTaskLogger().getPeriod(taskIndex).count());
  }
}

// test_multi_tasking_bike_system handler function
// cppcheck-suppress unusedFunction
static void test_multi_tasking_bike_system() {
  // create the BikeSystem instance
  multi_tasking::BikeSystem bikeSystem;

  // run the bike system in a separate thread
  Thread thread;
  thread.start(callback(&bikeSystem, &multi_tasking::BikeSystem::start));

  // let the bike system run for 20 secs
  ThisThread::sleep_for(20s);

  // stop the bike system
  bikeSystem.stop();

  // check whether scheduling was correct
  // Order is kGearTaskIndex, kSpeedTaskIndex, kTemperatureTaskIndex,
  //          kResetTaskIndex, kDisplayTask1Index, kDisplayTask2Index
  // When we use event handling, we do not check the computation time
  constexpr std::chrono::microseconds taskPeriods[] = {
      800000us, 400000us, 1600000us, 800000us, 1600000us, 1600000us};

  // allow for 2 msecs offset (with EventQueue)
  constexpr uint64_t kDeltaUs = 2000;
  TEST_ASSERT_UINT64_WITHIN(
      kDeltaUs,
      taskPeriods[advembsof::TaskLogger::kTemperatureTaskIndex].count(),
      bikeSystem.getTaskLogger()
          .getPeriod(advembsof::TaskLogger::kTemperatureTaskIndex)
          .count());
  TEST_ASSERT_UINT64_WITHIN(
      kDeltaUs,
      taskPeriods[advembsof::TaskLogger::kDisplayTask1Index].count(),
      bikeSystem.getTaskLogger()
          .getPeriod(advembsof::TaskLogger::kDisplayTask1Index)
          .count());
}

// test_reset_multi_tasking_bike_system handler function
Timer timer;
static std::chrono::microseconds resetTime = std::chrono::microseconds::zero();
static EventFlags eventFlags;
static constexpr uint32_t kResetEventFlag = (1UL << 0);
static void resetCallback() {
  resetTime = timer.elapsed_time();
  eventFlags.set(kResetEventFlag);
}

// cppcheck-suppress unusedFunction
static void test_reset_multi_tasking_bike_system() {
  // create the BikeSystem instance
  multi_tasking::BikeSystem bikeSystem;

  // run the bike system in a separate thread
  Thread thread;
  thread.start(callback(&bikeSystem, &multi_tasking::BikeSystem::start));

  // let the bike system run for 2 secs
  ThisThread::sleep_for(2s);

  // test reset on BikeSystem
  bikeSystem.getSpeedometer().setOnResetCallback(resetCallback);

  // start the timer instance
  timer.start();

  // check for reset response time
  constexpr uint8_t kNbrOfResets             = 10;
  std::chrono::microseconds lastResponseTime = std::chrono::microseconds::zero();
  for (uint8_t i = 0; i < kNbrOfResets; i++) {
    // take time before reset
    auto startTime = timer.elapsed_time();

    // reset the BikeSystem
    bikeSystem.onReset();

    // wait for resetCallback to be called
    eventFlags.wait_all(kResetEventFlag);

    // get the response time and check it
    auto responseTime = resetTime - startTime;

    printf("Reset task: response time is %lld usecs\n", responseTime.count());

    // cppcheck generates an internal error with 20us
    constexpr std::chrono::microseconds kMaxExpectedResponseTime(20);
    TEST_ASSERT_TRUE(responseTime.count() <= kMaxExpectedResponseTime.count());

    constexpr uint64_t kDeltaUs = 4;
    constexpr std::chrono::microseconds kMaxExpectedJitter(3);
    if (i > 0) {
      auto jitter = responseTime - lastResponseTime;
      TEST_ASSERT_UINT64_WITHIN(
          kDeltaUs, kMaxExpectedJitter.count(), std::abs(jitter.count()));
    }
    lastResponseTime = responseTime;

    // let the bike system run for 2 secs
    ThisThread::sleep_for(2s);
  }

  // stop the bike system
  bikeSystem.stop();
}

// cppcheck-suppress unusedFunction
static void test_gear_multi_tasking_bike_system() {
  // create the BikeSystem instance
  multi_tasking::BikeSystem bikeSystem;

  // run the bike system in a separate thread
  Thread thread;
  thread.start(callback(&bikeSystem, &multi_tasking::BikeSystem::start));

  // let the bike system run for 2 secs
  ThisThread::sleep_for(2s);

  // start the timer instance
  timer.start();

  // check for reset response time
  constexpr uint8_t kNbrOfGearChange = 10;
  for (uint8_t i = 0; i < kNbrOfGearChange; i++) {
    // take time before reset
    auto startTime = timer.elapsed_time();

    // increase current gear
    uint8_t currentGear = bikeSystem.getCurrentGear();
    bikeSystem.getGearDevice().onUp();

    // let the bike system run for 100 msecs
    ThisThread::sleep_for(100ms);

    uint8_t newCurrentGear = bikeSystem.getCurrentGear();
    if (currentGear == bike_computer::kMaxGear) {
      TEST_ASSERT_EQUAL_UINT8(currentGear, newCurrentGear);
    } else {
      TEST_ASSERT_EQUAL_UINT8(currentGear + 1, newCurrentGear);
    }

    // decrease current gear
    currentGear = bikeSystem.getCurrentGear();
    bikeSystem.getGearDevice().onDown();

    // let the bike system run for 100 msecs
    ThisThread::sleep_for(100ms);

    newCurrentGear = bikeSystem.getCurrentGear();
    if (currentGear == bike_computer::kMinGear) {
      TEST_ASSERT_EQUAL_UINT8(currentGear, newCurrentGear);
    } else {
      TEST_ASSERT_EQUAL_UINT8(currentGear - 1, newCurrentGear);
    }

    // let the bike system run for 2 secs
    ThisThread::sleep_for(2s);
  }

  // stop the bike system
  bikeSystem.stop();
}
#endif

ZTEST_SUITE(bike_system, NULL, NULL, NULL, NULL, NULL);
