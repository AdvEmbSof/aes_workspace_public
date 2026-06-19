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
#include <cerrno>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_string_conv.h>

// stl
#include <atomic>
#include <chrono>

// zpp_lib
#include "zpp_include/this_thread.hpp"
#include "zpp_include/zpp_assert.hpp"
#include "zpp_include/zpp_log.hpp"

ZPP_LOG_MODULE_REGISTER(sanitize, CONFIG_APP_LOG_LEVEL);

// for this example, we use c array on purpose
// NOLINTBEGIN(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays,cppcoreguidelines-pro-bounds-constant-array-index)
// constants for the lookup table
static constexpr int32_t kLutSize                        = 8;
static constexpr uint8_t kOffset                         = 1;
static constexpr int32_t kLut[kLutSize]                  = {10, 20, 30, 40, 50, 60, 70, 80};
static constexpr std::array<int32_t, kLutSize> kLutArray = {10, 20, 30, 40, 50, 60, 70, 80};

// pure c, with assertion for bounds checking
// when assertions are disabled, no bound checking is done
int32_t lookup_c_assert(int32_t sensor_value) {
  int32_t idx = sensor_value - kOffset;  // intended normalization
  // will abort if idx is out of bounds, but only when assertions are enabled
  ZPP_ASSERT(idx >= 0 && idx < kLutSize, "Index out of bounds: %d", idx);
  return kLut[idx];
}

// pure c, with clamping to valid range
static int32_t lookup_c_safe(int32_t sensor_value) {
  int32_t idx = sensor_value - kOffset;  // intended normalization
  if (idx < 0) {
    idx = 0;
  } else if (idx >= kLutSize) {
    idx = kLutSize - 1;
  }

  return kLut[idx];
}

// pure c++, using std::array for bounds checking
static int32_t lookup_cpp(int32_t sensor_value) {
  int32_t idx = sensor_value - kOffset;  // intended normalization
  return kLutArray.at(idx);              // will throw std::out_of_range if idx is out of bounds
}

// C++, use class with static method for lookup and fault counting
class LookUpTable {
public:
  static int32_t lookup(int32_t sensor_value) {
    int32_t idx = sensor_value - kOffset;  // intended normalization
    if (idx < 0 || idx >= kLutSize) {
      record_fault();
      return kFailSafeValue;
    }

    return kLut[idx];
  }

  static uint32_t fault_count() {
    return s_count;
  }

private:
  static constexpr int32_t kFailSafeValue = -1;  // or some other value indicating an error
  static void record_fault() {
    ++s_count;
  }
  static inline uint32_t s_count = 0;
};

static long parse_args(const struct shell* sh, size_t argc, char** argv) {
  if (argc != 2) {
    shell_error(sh, "Usage: sensor <lookup_function>");
    return -EINVAL;
  }

  static constexpr int kBase = 10;
  int parse_error            = 0;
  long parsed_lookup_value   = shell_strtol(argv[1], kBase, &parse_error);
  if (parse_error != 0) {
    shell_error(sh, "Invalid lookup function: %s", argv[1]);
    return -EINVAL;
  }

  return parsed_lookup_value;
}

// This is an internal function
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
static int32_t lookup_value(const struct shell* sh, int32_t sensor_value, long lookup_function) {
  switch (lookup_function) {
  case 0: {
    shell_print(sh, "Using lookup_c_assert");
    return lookup_c_assert(sensor_value);
  } break;
  case 1: {
    shell_print(sh, "Using lookup_c_safe");
    return lookup_c_safe(sensor_value);
  } break;
  case 2: {
    shell_print(sh, "Using lookup_cpp");
    return lookup_cpp(sensor_value);
  } break;
  case 3: {
    shell_print(sh, "Using LookUpTable");
    return LookUpTable::lookup(sensor_value);
  } break;
  default: {
    shell_error(sh, "Invalid lookup function: %ld", lookup_function);
    return -EINVAL;
  } break;
  }
  return -EINVAL;
}
// NOLINTEND(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays,cppcoreguidelines-pro-bounds-constant-array-index)

// This variable is used to simulate a sensor value that can be incremented or decremented via shell commands.
// In a real application, this value would come from an actual sensor reading and would not exist.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
std::atomic<int32_t> sensor_value(0);  // example sensor value, in a real application this would come from a sensor

static int cmd_inc(const struct shell* sh, size_t argc, char** argv) {
  long parsed_lookup_value = parse_args(sh, argc, argv);
  sensor_value++;
  int32_t lookup_result = lookup_value(sh, sensor_value.load(), parsed_lookup_value);
  shell_print(sh, "looking up sensor value: %d -> %d", sensor_value.load(), lookup_result);
  return 0;
}

static int cmd_dec(const struct shell* sh, size_t argc, char** argv) {
  long parsed_lookup_value = parse_args(sh, argc, argv);
  sensor_value--;
  int32_t lookup_result = lookup_value(sh, sensor_value.load(), parsed_lookup_value);
  shell_print(sh, "looking up sensor value: %d -> %d", sensor_value.load(), lookup_result);
  return 0;
}

// These are Zephyr macros that we have no control over, so we disable the linter for these lines
// NOLINTNEXTLINE(performance-no-int-to-ptr, cppcoreguidelines-pro-type-cstyle-cast, bugprone-branch-clone)
SHELL_CMD_ARG_REGISTER(inc, NULL, "Increment value <lookup_function>", cmd_inc, 2, 0);
// NOLINTNEXTLINE(performance-no-int-to-ptr, cppcoreguidelines-pro-type-cstyle-cast, bugprone-branch-clone)
SHELL_CMD_ARG_REGISTER(dec, NULL, "Increment value <lookup_function>", cmd_dec, 2, 0);

// The complexity is increased by zephyr macros
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
int main() {
  ZPP_LOG_DBG("Running on board %s", CONFIG_BOARD_TARGET);

  // do not return
  while (true) {
    using std::literals::chrono_literals::operator""s;
    static constexpr auto kTimeout = 1s;  // example sensor value
    zpp_lib::ThisThread::sleep_for(kTimeout);
  }

  return 0;
}
