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
 * @file sensor_device.hpp
 * @author Serge Ayer <serge.ayer@hefr.ch>
 *
 * @brief SensorDevice header file
 *
 * @date 2025-07-01
 * @version 1.0.0
 ***************************************************************************/

#pragma once

// zephyr
#include <zephyr/kernel.h>

// zpp_lib
#include "zpp_include/non_copyable.hpp"
#include "zpp_include/zephyr_result.hpp"

namespace bike_computer {

class SensorDevice : private zpp_lib::NonCopyable<SensorDevice> {
 public:
  // constructor
  SensorDevice() = default;

  // method for initializing the device
  [[nodiscard]] zpp_lib::ZephyrResult initialize();

  // methods used for reading sensor measurements
  [[nodiscard]] zpp_lib::ZephyrResult readTemperature(float& temperature);
  [[nodiscard]] zpp_lib::ZephyrResult readHumidity(float& humidity);

 private:
  // data members
  const struct device* _sensorDevice = nullptr;
};

}  // namespace bike_computer
