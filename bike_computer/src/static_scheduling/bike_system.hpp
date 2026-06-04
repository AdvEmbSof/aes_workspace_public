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
 * @file bike_system.hpp
 * @author Serge Ayer <serge.ayer@hefr.ch>
 *
 * @brief Bike System header file (static scheduling)
 *
 * @date 2025-07-01
 * @version 1.0.0
 ***************************************************************************/

#pragma once

// stl
#include <atomic>

// local
#include "gear_device.hpp"
#include "pedal_device.hpp"
#include "reset_device.hpp"

// zpp_lib
#include "zpp_include/display.hpp"

// from common
#include "common/bike_display.hpp"
#include "common/sensor_device.hpp"
#include "common/speedometer.hpp"
#include "common/task_manager.hpp"

namespace bike_computer::static_scheduling {

class BikeSystem : private zpp_lib::NonCopyable<BikeSystem> {
public:
  // constructor
  BikeSystem() = default;

  // method called in main() for starting the system
  [[nodiscard]] zpp_lib::ZephyrResult start();

  // method called for stopping the system
  void stop();

private:
  // private methods
  [[nodiscard]] zpp_lib::ZephyrResult initialize();
  void gear_task();
  void speed_distance_task();
  void temperature_task();
  void reset_task();
  void display_task1();
  void display_task2();

  // flag stating whether sleep is allows when simulating computation times
  static constexpr bool kAllowSleep = false;
  // stop flag, used for stopping the super-loop (set in stop())
  volatile std::atomic<bool> _stop_flag = false;
  // data member that represents the device for manipulating the gear
  GearDevice _gear_device;
  uint8_t _current_gear      = bike_computer::kMinGear;
  uint8_t _current_gear_size = bike_computer::kMinGearSize;
  // data member that represents the device for manipulating the pedal rotation
  // speed/time
  PedalDevice _pedal_device;
  float _current_speed     = 0.0F;
  float _traveled_distance = 0.0F;
  // data member that represents the device used for resetting
  ResetDevice _reset_device;
  // data member that represents the display
  BikeDisplay _bike_display;
  // data member that represents the device for counting wheel rotations
  Speedometer _speedometer;
  // data member that represents the sensor device
  SensorDevice _sensor_device;
  float _current_temperature = 0.0F;

  // used for managing tasks info
  TaskManager _task_manager;
};

} // namespace bike_computer::static_scheduling
