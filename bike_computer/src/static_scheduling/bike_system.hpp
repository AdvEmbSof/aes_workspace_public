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

namespace bike_computer {

namespace static_scheduling {

class BikeSystem : private zpp_lib::NonCopyable<BikeSystem> {
 public:
  // constructor
  BikeSystem() = default;

  // method called in main() for starting the system
  [[nodiscard]] zpp_lib::ZephyrResult start();
  [[nodiscard]] zpp_lib::ZephyrResult startTTCE();

  // method called for stopping the system
  void stop();

 private:
  // private methods
  [[nodiscard]] zpp_lib::ZephyrResult initialize();
  void gearTask();
  void speedDistanceTask();
  void temperatureTask();
  void resetTask();
  void displayTask1();
  void displayTask2();

  // stop flag, used for stopping the super-loop (set in stop())
  atomic_t _stopFlag = ATOMIC_INIT(0x00);
  // data member that represents the device for manipulating the gear
  GearDevice _gearDevice;
  uint8_t _currentGear     = bike_computer::kMinGear;
  uint8_t _currentGearSize = bike_computer::kMinGearSize;
  // data member that represents the device for manipulating the pedal rotation
  // speed/time
  PedalDevice _pedalDevice;
  float _currentSpeed     = 0.0f;
  float _traveledDistance = 0.0f;
  // data member that represents the device used for resetting
  ResetDevice _resetDevice;
  // data member that represents the display
  BikeDisplay _bikeDisplay;
  // data member that represents the device for counting wheel rotations
  Speedometer _speedometer;
  // data member that represents the sensor device
  SensorDevice _sensorDevice;
  float _currentTemperature = 0.0f;

  // used for managing tasks info
  TaskManager _taskManager;
};

}  // namespace static_scheduling

}  // namespace bike_computer
