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
 * @file main.cpp
 * @author Serge Ayer <serge.ayer@hefr.ch>
 *
 * @brief Main function of the bme280 program
 *
 * @date 2025-07-01
 * @version 1.0.0
 ***************************************************************************/

// stl
#include <chrono>

// zpp-lib
#include "zpp_include/this_thread.hpp"
#include "zpp_include/thread.hpp"

// zephyr
#if !CONFIG_EMUL
#include <zephyr/drivers/sensor.h>
#endif  // !CONFIG_EMUL

// zpp-lib
#include "zpp_include/zpp_log.hpp"

ZPP_LOG_MODULE_REGISTER(sensor_bm280, CONFIG_APP_LOG_LEVEL);

void read_sensor() {
#if !CONFIG_EMUL
  // NOLINTNEXTLINE(cppcoreguidelines-init-variables)
  const struct device* sensor_device = DEVICE_DT_GET(DT_INST(0, bosch_bme280));

  if (!device_is_ready(sensor_device)) {
    ZPP_LOG_ERR("Device %s not found", sensor_device->name);
    return;
  }
  struct sensor_value temperature_sv = {};
  struct sensor_value humidity_sv    = {};
  struct sensor_value pressure_sv    = {};
#endif  // !CONFIG_EMUL

  using namespace std::literals;
  static constexpr std::chrono::milliseconds kReadInterval = 1000ms;

  while (true) {
#if !CONFIG_EMUL
    sensor_sample_fetch(sensor_device);
    sensor_channel_get(sensor_device, SENSOR_CHAN_AMBIENT_TEMP, &temperature_sv);
    sensor_channel_get(sensor_device, SENSOR_CHAN_HUMIDITY, &humidity_sv);
    sensor_channel_get(sensor_device, SENSOR_CHAN_PRESS, &pressure_sv);
    ZPP_LOG_INF("T=%.2f [deg C] P=%.2f [kPa] H=%.1f [%%]",
                sensor_value_to_double(&temperature_sv),
                sensor_value_to_double(&pressure_sv),
                sensor_value_to_double(&humidity_sv));
#else
    // Emulated sensor values
    ZPP_LOG_INF("T=%.2f [deg C] P=%.2f [kPa] H=%.1f [%%]", 25.0, 50.0, 1013.0);
#endif  // !CONFIG_EMUL

    zpp_lib::ThisThread::sleep_for(kReadInterval);
  }
}

int main() {

  ZPP_LOG_DBG("Running on board %s", CONFIG_BOARD_TARGET);

  zpp_lib::Thread thread(zpp_lib::PreemptableThreadPriority::PriorityNormal, "Blinky");
  auto res = thread.start(read_sensor);
  if (!res) {
    return -1;
  }

  res = thread.join();
  if (!res) {
    ZPP_LOG_ERR("Could not join thread: %d", (int)res.error());
    return -1;
  }

  return 0;
}
