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
#include <zephyr/logging/log.h>

// std
#include <chrono>
#include <ratio>

// zpp_lib
#include "zpp_include/time.hpp"

LOG_MODULE_DECLARE(bike_computer, CONFIG_APP_LOG_LEVEL);

namespace bike_computer {

Speedometer::Speedometer() : _lastTime(zpp_lib::Time::getUpTime()) {}

void Speedometer::setCurrentRotationTime(
    const std::chrono::milliseconds& currentRotationTime) {
  if (_pedalRotationTime != currentRotationTime) {
    // compute distance before changing the rotation time
    computeDistance();

    // change pedal rotation time
    _pedalRotationTime = currentRotationTime;

    // compute speed with the new pedal rotation time
    computeSpeed();
  }
}

void Speedometer::setGearSize(uint8_t gearSize) {
  if (_gearSize != gearSize) {
    // compute distance before chaning the gear size
    computeDistance();

    // change gear size
    _gearSize = gearSize;

    // compute speed with the new gear size
    computeSpeed();
  }
}

float Speedometer::getCurrentSpeed() const { return _currentSpeed; }

float Speedometer::getDistance() {
  // make sure to update the distance traveled
  computeDistance();
  return _totalDistance;
}

void Speedometer::reset() {
#if CONFIG_TEST == 1
  if (_cb != nullptr) {
    _cb();
  }
#endif  // CONFIG_TEST == 1

  // TODO
}

#if CONFIG_TEST == 1
uint8_t Speedometer::getGearSize() const { return _gearSize; }

float Speedometer::getWheelCircumference() const { return kWheelCircumference; }

float Speedometer::getTraySize() const { return kTraySize; }

std::chrono::milliseconds Speedometer::getCurrentPedalRotationTime() const {
  return _pedalRotationTime;
}

void Speedometer::setOnResetCallback(std::function<void()> cb) { _cb = cb; }

#endif  // CONFIG_TEST == 1

void Speedometer::computeSpeed() {
  // For computing the speed given a rear gear (braquet), one must divide the size of
  // the tray (plateau) by the size of the rear gear (pignon arrière), and then multiply
  // the result by the circumference of the wheel. Example: tray = 50, rear gear = 15.
  // Distance run with one pedal turn (wheel circumference = 2.10 m) = 50/15 * 2.1 m
  // = 6.99m If you ride at 80 pedal turns / min, you run a distance of 6.99 * 80 / min
  // ~= 560 m / min = 33.6 km/h

  // TODO
}

void Speedometer::computeDistance() {
  // For computing the speed given a rear gear (braquet), one must divide the size of
  // the tray (plateau) by the size of the rear gear (pignon arrière), and then multiply
  // the result by the circumference of the wheel. Example: tray = 50, rear gear = 15.
  // Distance run with one pedal turn (wheel circumference = 2.10 m) = 50/15 * 2.1 m
  // = 6.99m If you ride at 80 pedal turns / min, you run a distance of 6.99 * 80 / min
  // ~= 560 m / min = 33.6 km/h. We then multiply the speed by the time for getting the
  // distance traveled.

  // TODO
}

}  // namespace bike_computer
