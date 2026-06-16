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
 * @brief Main function of the Blinky program
 *
 * @date 2025-07-01
 * @version 1.0.0
 ***************************************************************************/

// zephyr
#include <zephyr/shell/shell.h>

// stl
#include <chrono>
#include <atomic>

// zpp_lib
#include "zpp_include/this_thread.hpp"
#include "zpp_include/zpp_assert.hpp"
#include "zpp_include/zpp_log.hpp"

ZPP_LOG_MODULE_REGISTER(sanitize, CONFIG_APP_LOG_LEVEL);

// constants for the lookup table
static constexpr int32_t kLutSize = 8;
static constexpr uint8_t kOffset = 1;
static constexpr int32_t kLut[kLutSize] = {10, 20, 30, 40, 50, 60, 70, 80};
static constexpr std::array<int32_t, kLutSize> kLutArray = {10, 20, 30, 40, 50, 60, 70, 80};

// pure c, with clamping to valid range
int32_t lookup_c_safe(int8_t sensor_value) {
  int32_t idx = sensor_value - kOffset;   // intended normalization
  if (idx < 0) {
    idx = 0;
  } else if (idx >= kLutSize) {
    idx = kLutSize - 1;
  }

  return kLut[idx];
}

// pure c, with assertion for bounds checking
// when assertions are disabled, no bound checking is done
int32_t lookup_c_assert(int8_t sensor_value) {
  int32_t idx = sensor_value - kOffset;   // intended normalization
  // will abort if idx is out of bounds, but only when assertions are enabled
  ZPP_ASSERT(idx >= 0 && idx < kLutSize, "Index out of bounds: %d", idx); 
  return kLut[idx];
}

// pure c++, using std::array for bounds checking
int32_t lookup_cpp(int8_t sensor_value) {
  int32_t idx = sensor_value - kOffset;   // intended normalization
  return kLutArray.at(idx); // will throw std::out_of_range if idx is out of bounds
}

// C++, use class with static method for lookup and fault counting
class LookUpTable
{
public:
  static int32_t lookup(int32_t sensor_value) {
    const int32_t idx = sensor_value - kOffset; // intended normalization
    if (idx < 0 || idx >= kLutSize) {
      record_fault();
      return kFailSafeValue;
    }

    return kLut[idx];
  }

  static uint32_t fault_count() {
    return _fault_count;
  }

private:
  static constexpr int32_t kFailSafeValue = -1; // or some other value indicating an error
  static void record_fault() {
    ++_fault_count;
  }
  static inline uint32_t _fault_count = 0;  
};


void process_sensor_value(const struct shell *sh, int32_t value) {
  // placeholder for processing the sensor value
  shell_print(sh, "Processing sensor value: %d", value);
  // shell_print(sh, "(lookup_c_safe: looking up sensor value: %d -> %d", value, lookup_c_safe(value));
  // shell_print(sh, "(lookup_c_assert: looking up sensor value: %d -> %d", value, lookup_c_assert(value));
  // shell_print(sh, "(lookup_cpp: looking up sensor value: %d -> %d", value, lookup_cpp(value));
  shell_print(sh, "(ProductionLookUpTable: looking up sensor value: %d -> %d (# faults: %d)", value, LookUpTable::lookup(value), LookUpTable::fault_count());
  // shell_print(sh, "(lookup_c_separate(kSensorValueExample): looking up sensor value: %d -> %d", kSensorValueExample, lookup_c_separate(kSensorValueExample));
  // shell_print(sh, "(lookup_cpp(kSensorValueExample): looking up sensor value: %d -> %d", kSensorValueExample, lookup_cpp(kSensorValueExample));
  // shell_print(sh, "(lookup_c_unsafe(kSensorValueExample): looking up sensor value: %d -> %d", kSensorValueExample, lookup_c_unsafe(kSensorValueExample));
  // LookUpTable::lookup(kSensorValueExample);
  // ZPP_LOG_DBG("ProductionLookUpTable: looking up sensor value: %d -> %d", kSensorValueExample, ProductionLookUpTable::lookup(kSensorValueExample));
}

std::atomic<int32_t> sensor_value(0); // example sensor value, in a real application this would come from a sensor

static int cmd_inc(const struct shell *sh, size_t argc, char **argv)
{
    sensor_value++;
    shell_print(sh, "value = %d", sensor_value.load());
    process_sensor_value(sh, sensor_value.load());
    return 0;
}

static int cmd_dec(const struct shell *sh, size_t argc, char **argv)
{
    sensor_value--;
    shell_print(sh, "value = %d", sensor_value.load());
    process_sensor_value(sh, sensor_value.load());
    return 0;
}

SHELL_CMD_REGISTER(inc, NULL, "Increment value", cmd_inc);
SHELL_CMD_REGISTER(dec, NULL, "Decrement value", cmd_dec);

#if 0
void on_button1(std::atomic<int32_t>& sensor_value) {
  int32_t local_value = ++sensor_value;
  ZPP_LOG_INF("Button 1 pressed: sensor value is now %d", local_value);
  process_sensor_value(local_value);
}

void on_button2(std::atomic<int32_t>& sensor_value) {
  int32_t local_value = --sensor_value;
  ZPP_LOG_INF("Button 2 pressed: sensor value is now %d", local_value);
  process_sensor_value(local_value);
}
#endif

// The complexity is increased by zephyr macros
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
int main() {
  ZPP_LOG_DBG("Running on board %s", CONFIG_BOARD_TARGET);
  
#if 0
  std::atomic<int32_t> sensor_value(0); // example sensor value, in a real application this would come from a sensor
  zpp_lib::InterruptIn button1(zpp_lib::InterruptIn::PinName::BUTTON1);
  button1.fall([&sensor_value]() { on_button1(sensor_value); });
 
  zpp_lib::InterruptIn button2(zpp_lib::InterruptIn::PinName::BUTTON2);
  button2.fall([&sensor_value]() { on_button2(sensor_value); });
#endif


  // do not return
  while (true) {
    using std::literals::chrono_literals::operator""s;
    static constexpr auto kTimeout = 1s;  // example sensor value
    zpp_lib::ThisThread::sleep_for(kTimeout);
  }

  return 0;
}
