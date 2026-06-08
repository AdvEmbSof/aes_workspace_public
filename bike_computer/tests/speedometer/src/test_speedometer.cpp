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
 * @file test_speedometer.cpp
 * @author Serge Ayer <serge.ayer@hefr.ch>
 *
 * @brief Test program for the Speedometer class
 *
 * @date 2025-07-01
 * @version 1.0.0
 ***************************************************************************/

// zephyr
#include <zephyr/ztest.h>

// std
#include <chrono>
#include <cstdio>

// zpp_lib
#include "zpp_include/this_thread.hpp"
#include "zpp_include/zpp_assert.hpp"
#include "zpp_include/zpp_log.hpp"

// bike_computer
#include "common/speedometer.hpp"

ZPP_LOG_MODULE_REGISTER(bike_computer, CONFIG_APP_LOG_LEVEL);

// allow for 0.1 km/h difference
static constexpr float kAllowedSpeedDelta = 0.1F;
// allow for 1m difference
static constexpr float kAllowedDistanceDelta = 1.0F / 1000.0F;

// function called by test handler functions for verifying the current speed
// Internal function
void check_current_speed(const std::chrono::milliseconds& pedal_rotation_time,
                         uint8_t tray_size,
                         uint8_t gear_size,          // NOLINT(bugprone-easily-swappable-parameters)
                         float wheel_circumference,  // NOLINT(bugprone-easily-swappable-parameters)
                         float current_speed) {
  // compute the number of pedal rotation per hour
  uint32_t milliseconds_per_hour = 1000 * 3600;
  float pedal_rotations_per_hour = static_cast<float>(milliseconds_per_hour) / static_cast<float>(pedal_rotation_time.count());

  // compute the expected speed in km / h
  // first compute the distance in meter for each pedal turn
  float tray_gear_ratio         = static_cast<float>(tray_size) / static_cast<float>(gear_size);
  float distance_per_pedal_turn = tray_gear_ratio * wheel_circumference;
  float expected_speed          = (distance_per_pedal_turn / 1000.0F) * pedal_rotations_per_hour;

  ZPP_LOG_INF("  Expected speed is %f, current speed is %f\n", static_cast<double>(expected_speed), static_cast<double>(current_speed));
  zpp_zassert_within(current_speed, expected_speed, kAllowedSpeedDelta, "Current speed is not within bounds");
}

// compute the traveled distance for a time interval
// Internal function
float compute_distance(const std::chrono::milliseconds& pedal_rotation_time,
                       uint8_t tray_size,
                       uint8_t gear_size,  // NOLINT(bugprone-easily-swappable-parameters)
                       float wheel_circumference,
                       const std::chrono::milliseconds& travel_time) {
  // compute the number of pedal rotation during travel time
  // both times are expressed in ms
  float pedal_rotations = static_cast<float>(travel_time.count()) / static_cast<float>(pedal_rotation_time.count());

  // compute the distance in meter for each pedal turn
  float tray_gear_ratio         = static_cast<float>(tray_size) / static_cast<float>(gear_size);
  float distance_per_pedal_turn = tray_gear_ratio * wheel_circumference;

  // distancePerPedalTurn is expressed in m, divide per 1000 for a distance in km
  return (distance_per_pedal_turn * pedal_rotations) / 1000.0F;
}

// function called by test handler functions for verifying the distance traveled
void check_distance(const std::chrono::milliseconds& pedal_rotation_time,
                    uint8_t tray_size,
                    uint8_t gear_size,
                    float wheel_circumference,
                    const std::chrono::milliseconds& travel_time,
                    float distance) {
  // distancePerPedalTurn is expressed in m, divide per 1000 for a distance in km
  float expected_distance = compute_distance(pedal_rotation_time, tray_size, gear_size, wheel_circumference, travel_time);
  ZPP_LOG_INF("  Expected distance is %f, current distance is %f\n", static_cast<double>(expected_distance), static_cast<double>(distance));
  zpp_zassert_within(distance, expected_distance, kAllowedDistanceDelta, "Current distance is not within bounds");
}

// test the speedometer by modifying the gear
ZTEST(speedometer, test_gear_size) {
  // create a speedometer instance
  bike_computer::Speedometer speedometer;

  // get speedometer constant values (for this test)
  auto tray_size           = speedometer.get_tray_size();
  auto wheel_circumference = speedometer.get_wheel_circumference();
  auto pedal_rotation_time = speedometer.get_current_pedal_rotation_time();

  for (uint8_t gear_size = bike_computer::kMinGearSize; gear_size <= bike_computer::kMaxGearSize; gear_size++) {
    // set the gear
    ZPP_LOG_INF("Testing gear size %d\n", gear_size);
    speedometer.set_gear_size(gear_size);

    // get the current speed
    auto current_speed = speedometer.get_current_speed();

    // check the speed against the expected one
    check_current_speed(pedal_rotation_time, tray_size, gear_size, wheel_circumference, current_speed);
  }
}

// test the speedometer by modifying the pedal rotation speed
ZTEST(speedometer, test_rotation_speed) {
  // create a speedometer instance
  bike_computer::Speedometer speedometer;

  // set the gear size
  speedometer.set_gear_size(bike_computer::kMaxGearSize);

  // get speedometer constant values
  auto tray_size           = speedometer.get_tray_size();
  auto wheel_circumference = speedometer.get_wheel_circumference();
  auto gear_size           = speedometer.get_gear_size();

  // first test increasing rotation speed (decreasing rotation time)
  auto pedal_rotation_time = speedometer.get_current_pedal_rotation_time();
  while (pedal_rotation_time > bike_computer::kMinPedalRotationTime) {
    // decrease the pedal rotation time
    pedal_rotation_time -= bike_computer::kDeltaPedalRotationTime;
    speedometer.set_current_pedal_rotation_time(pedal_rotation_time);

    // get the current speed
    auto current_speed = speedometer.get_current_speed();

    // check the speed against the expected one
    check_current_speed(pedal_rotation_time, tray_size, gear_size, wheel_circumference, current_speed);
  }

  // second test decreasing rotation speed (increasing rotation time)
  pedal_rotation_time = speedometer.get_current_pedal_rotation_time();
  while (pedal_rotation_time < bike_computer::kMaxPedalRotationTime) {
    // increase the pedal rotation time
    pedal_rotation_time += bike_computer::kDeltaPedalRotationTime;
    speedometer.set_current_pedal_rotation_time(pedal_rotation_time);

    // get the current speed
    auto current_speed = speedometer.get_current_speed();

    // check the speed against the expected one
    check_current_speed(pedal_rotation_time, tray_size, gear_size, wheel_circumference, current_speed);
  }
}

// test the speedometer by modifying the pedal rotation speed
ZTEST(speedometer, test_distance) {
  // create a speedometer instance
  bike_computer::Speedometer speedometer;

  // set the gear size
  speedometer.set_gear_size(bike_computer::kMaxGearSize);

  // get speedometer constant values
  auto tray_size           = speedometer.get_tray_size();
  auto wheel_circumference = speedometer.get_wheel_circumference();
  auto gear_size           = speedometer.get_gear_size();
  auto pedal_rotation_time = speedometer.get_current_pedal_rotation_time();

  // test different travel times
  using std::literals::chrono_literals::operator""s;
  using std::literals::chrono_literals::operator""ms;
  static constexpr std::array<std::chrono::milliseconds, 4> kTravelTimes = {500ms, 1000ms, 5s, 10s};

  // first check travel distance without changing gear and rotation speed
  auto total_travel_time = std::chrono::milliseconds::zero();
  for (auto travel_time : kTravelTimes) {
    // run for the travel time and get the distance
    zpp_lib::ThisThread::sleep_for(travel_time);

    // get the traveled distance
    auto traveled_distance = speedometer.get_traveled_distance();

    // accumulate travel time
    total_travel_time += travel_time;

    // check the distance vs the expected one
    check_distance(pedal_rotation_time, tray_size, gear_size, wheel_circumference, total_travel_time, traveled_distance);
  }

  // now change gear at each time interval
  auto expected_distance = speedometer.get_traveled_distance();
  for (auto travel_time : kTravelTimes) {
    // update the gear size
    gear_size++;
    speedometer.set_gear_size(gear_size);

    // run for the travel time and get the distance
    zpp_lib::ThisThread::sleep_for(travel_time);

    // compute the expected distance for this time segment
    float distance = compute_distance(pedal_rotation_time, tray_size, gear_size, wheel_circumference, travel_time);
    expected_distance += distance;

    // get the distance traveled
    auto traveled_distance = speedometer.get_traveled_distance();

    printk("  Expected distance is %f, current distance is %f\n",
           static_cast<double>(expected_distance),
           static_cast<double>(traveled_distance));
    zpp_zassert_within(traveled_distance, expected_distance, kAllowedDistanceDelta);
  }

  // now change rotation speed at each time interval
  expected_distance = speedometer.get_traveled_distance();
  for (auto travel_time : kTravelTimes) {
    // update the rotation speed
    pedal_rotation_time += bike_computer::kDeltaPedalRotationTime;
    speedometer.set_current_pedal_rotation_time(pedal_rotation_time);

    // run for the travel time and get the distance
    zpp_lib::ThisThread::sleep_for(travel_time);

    // compute the expected distance for this time segment
    float distance = compute_distance(pedal_rotation_time, tray_size, gear_size, wheel_circumference, travel_time);
    expected_distance += distance;

    // get the distance traveled
    auto traveled_distance = speedometer.get_traveled_distance();

    printk("  Expected distance is %f, current distance is %f\n",
           static_cast<double>(expected_distance),
           static_cast<double>(traveled_distance));
    zpp_zassert_within(traveled_distance, expected_distance, kAllowedDistanceDelta);
  }
}

// test the speedometer by modifying the pedal rotation speed
ZTEST(speedometer, test_reset) {
  // create a speedometer instance
  bike_computer::Speedometer speedometer;

  // set the gear size
  speedometer.set_gear_size(bike_computer::kMinGearSize);

  // get speedometer constant values
  auto tray_size           = speedometer.get_tray_size();
  auto wheel_circumference = speedometer.get_wheel_circumference();
  auto gear_size           = speedometer.get_gear_size();
  auto pedal_rotation_time = speedometer.get_current_pedal_rotation_time();

  // travel for 5 seconds
  using std::literals::chrono_literals::operator""ms;
  const auto kTravelTime = 5000ms;
  zpp_lib::ThisThread::sleep_for(kTravelTime);

  // check the expected distaance traveled
  auto expected_distance = compute_distance(pedal_rotation_time, tray_size, gear_size, wheel_circumference, kTravelTime);

  // get the distance traveled
  auto traveled_distance = speedometer.get_traveled_distance();

  ZPP_LOG_INF("  Expected distance is %f, current distance is %f\n",
              static_cast<double>(expected_distance),
              static_cast<double>(traveled_distance));
  zpp_zassert_within(traveled_distance, expected_distance, kAllowedDistanceDelta);

  // reset the speedometer
  speedometer.reset();

  // traveled distance should now be zero
  traveled_distance = speedometer.get_traveled_distance();

  ZPP_LOG_INF("  Expected distance is %f, current distance is %f\n", 0.0, static_cast<double>(traveled_distance));
  zpp_zassert_within(0.0F, traveled_distance, kAllowedDistanceDelta);

  // travel again for 5 seconds
  zpp_lib::ThisThread::sleep_for(kTravelTime);

  // reset the speedometer without getting the distance
  speedometer.reset();

  // travel again for 5 seconds
  zpp_lib::ThisThread::sleep_for(kTravelTime);

  // get the distance traveled
  traveled_distance = speedometer.get_traveled_distance();

  ZPP_LOG_INF("  Expected distance is %f, current distance is %f\n",
              static_cast<double>(expected_distance),
              static_cast<double>(traveled_distance));
  zpp_zassert_within(traveled_distance, expected_distance, kAllowedDistanceDelta);
}

ZTEST_SUITE(speedometer, NULL, NULL, NULL, NULL, NULL);
