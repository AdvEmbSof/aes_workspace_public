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
 * @file speedometer_device.cpp
 * @author Serge Ayer <serge.ayer@hefr.ch>
 *
 * @brief Speedometer implementation
 *
 * @date 2025-07-01
 * @version 1.0.0
 ***************************************************************************/

#include "speedometer.hpp"

// zephyr

// std
#include <chrono>

// zpp_lib
#include "zpp_include/time.hpp"
#include "zpp_include/zpp_assert.hpp"
#include "zpp_include/zpp_log.hpp"

ZPP_LOG_MODULE_DECLARE(bike_computer, CONFIG_APP_LOG_LEVEL);

namespace bike_computer {

Speedometer::Speedometer() : _last_time(zpp_lib::Time::get_uptime()) {}

void Speedometer::set_current_pedal_rotation_time(const std::chrono::milliseconds& current_rotation_time) {
  if (_pedal_rotation_time != current_rotation_time) {
    // compute distance before changing the rotation time
    compute_traveled_distance();

    // change pedal rotation time
    _pedal_rotation_time = current_rotation_time;

    // compute speed with the new pedal rotation time
    compute_speed();
  }
}

void Speedometer::set_gear_size(uint8_t gear_size) {
  if (_gear_size != gear_size) {
    // compute distance before changing the gear size
    compute_traveled_distance();

    // change gear size
    _gear_size = gear_size;

    // compute speed with the new gear size
    compute_speed();
  }
}

float Speedometer::get_current_speed() const {
  return _current_speed;
}

float Speedometer::get_traveled_distance() {
  // make sure to update the distance traveled
  return compute_traveled_distance();
}

void Speedometer::reset() {
#if CONFIG_TEST
  if (_cb != nullptr) {
    _cb();
  }
#endif  // CONFIG_TEST
  // TODO(Student)
    
}

#if CONFIG_TEST
uint8_t Speedometer::get_gear_size() const {
  return _gear_size;
}

float Speedometer::get_wheel_circumference() const {
  return kWheelCircumference;
}

uint8_t Speedometer::get_tray_size() const {
  return kTraySize;
}

std::chrono::milliseconds Speedometer::get_current_pedal_rotation_time() const {
  return _pedal_rotation_time;
}

void Speedometer::set_on_reset_callback(CallbackFunction cb) {
  _cb = cb;
}

#endif  // CONFIG_TEST

void Speedometer::compute_speed() {
  // For computing the speed given a rear gear (braquet), one must divide the size of
  // the tray (plateau) by the size of the rear gear (pignon arrière), and then multiply
  // the result by the circumference of the wheel. Example: tray = 50, rear gear = 15.
  // Distance run with one pedal turn (wheel circumference = 2.10 m) = 50/15 * 2.1 m
  // = 6.99m If you ride at 80 pedal turns / min, you run a distance of 6.99 * 80 / min
  // ~= 560 m / min = 33.6 km/h

  // TODO(Student)

float Speedometer::compute_traveled_distance() {
  // For computing the speed given a rear gear (braquet), one must divide the size of
  // the tray (plateau) by the size of the rear gear (pignon arrière), and then multiply
  // the result by the circumference of the wheel. Example: tray = 50, rear gear = 15.
  // Distance run with one pedal turn (wheel circumference = 2.10 m) = 50/15 * 2.1 m
  // = 6.99m If you ride at 80 pedal turns / min, you run a distance of 6.99 * 80 / min
  // ~= 560 m / min = 33.6 km/h. We then multiply the speed by the time for getting the
  // distance traveled.

  // TODO(Student)
  

}  // namespace bike_computer
