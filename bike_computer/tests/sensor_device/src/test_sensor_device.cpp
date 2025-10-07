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
 * @file test_sensor_device.cpp
 * @author Serge Ayer <serge.ayer@hefr.ch>
 *
 * @brief Test program for the SensorDevice class
 *
 * @date 2025-07-01
 * @version 1.0.0
 ***************************************************************************/

// zephyr
#include <zephyr/logging/log.h>
#include <zephyr/ztest.h>

// bike_computer
#include "common/sensor_device.hpp"

LOG_MODULE_REGISTER(test_sensor_device, CONFIG_APP_LOG_LEVEL);

ZTEST(sensor_device, test_sensor_device) {
  // create the SensorDevice instance
  bike_computer::SensorDevice sensorDevice;

  auto res = sensorDevice.initialize();
  if (!res) {
    zassert_true(res, "Cannot initialize sensor device: %d", res.error());
  }

  float temperature = 0.0f;
  res               = sensorDevice.readTemperature(temperature);
  if (!res) {
    zassert_true(res, "Cannot initialize sensor device: %d", res.error());
  }
  static constexpr float kTemperatureRange = 20.0f;
  static constexpr float kMeanTemperature  = 15.0f;
  zassert_within(temperature,
                 kMeanTemperature,
                 kTemperatureRange,
                 "Temperature outside range: %f",
                 static_cast<double>(temperature));

  float humidity = 0.0f;
  res            = sensorDevice.readTemperature(humidity);
  if (!res) {
    zassert_true(res, "Cannot initialize sensor device: %d", res.error());
  }
  static constexpr float kHumidityRange = 45.0f;
  static constexpr float kMeanHumidity  = 50.0f;
  zassert_within(humidity,
                 kMeanHumidity,
                 kHumidityRange,
                 "Humidity outside range: %f",
                 static_cast<double>(humidity));
}

ZTEST_SUITE(sensor_device, NULL, NULL, NULL, NULL, NULL);
