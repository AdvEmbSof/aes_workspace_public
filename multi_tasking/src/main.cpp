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
 * @brief Main function of the Multi-Tasking program
 *
 * @date 2025-07-01
 * @version 1.0.0
 ***************************************************************************/

// zephyr
#include <zephyr/kernel.h>
#include <zephyr/random/random.h>

// std
#include <ostream>

// zpp_lib
#include "zpp_include/this_thread.hpp"
#include "zpp_include/utils.hpp"
#include "zpp_include/zpp_log.hpp"

// local
#include "buffer_solution.hpp"
#include "clock_with_mutex.hpp"
#include "consumer.hpp"
#include "deadlock.hpp"
#include "producer.hpp"
#include "wait_on_button.hpp"

ZPP_LOG_MODULE_REGISTER(multi_tasking, CONFIG_APP_LOG_LEVEL);

class RandomIntGenerator {
public:
  static constexpr uint8_t kMaxRandomValue = 20;

  static uint32_t s_produce_next_value() {
    return sys_rand32_get() % kMaxRandomValue;
  }
};

class RandomDoubleGenerator {
public:
  // kRandomValues is a static array, so using a c array is safe
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  static constexpr double kRandomValues[] = {1.1, 2.2, 3.3, 4.4, 5.5};
  static double s_produce_next_value() {
    // kRandomValues is a static array, so we are sure that the index is within bounds
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    return kRandomValues[sys_rand32_get() % (sizeof(kRandomValues) / sizeof(kRandomValues[0]))];
  }
};

struct Rect {
  int32_t x;
  int32_t y;
};

std::ostream& operator<<(std::ostream& os, const Rect& rect) {
  os << "(" << rect.y << ", " << rect.x << ")";
  return os;
}

class RandomRectGenerator {
public:
  // kRandomValues is a static array, so using a c array is safe
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  static constexpr Rect kRandomValues[] = {{.x = 1, .y = 1}, {.x = 2, .y = 2}, {.x = 3, .y = 3}, {.x = 4, .y = 4}, {.x = 5, .y = 5}};
  static Rect s_produce_next_value() {
    // kRandomValues is a static array, so we are sure that the index is within bounds
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    return kRandomValues[sys_rand32_get() % (sizeof(kRandomValues) / sizeof(kRandomValues[0]))];
  }
};

// Complexity is increased by the use of zephyr macros
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
int main() {
  using std::literals::chrono_literals::operator""ms;

  ZPP_LOG_DBG("Multi-tasking program started");

  // check which button is pressed
  zpp_lib::InterruptIn button1(zpp_lib::InterruptIn::PinName::BUTTON1);
  zpp_lib::InterruptIn button2(zpp_lib::InterruptIn::PinName::BUTTON2);
  zpp_lib::InterruptIn button3(zpp_lib::InterruptIn::PinName::BUTTON3);
  if (button1.read() == zpp_lib::kPolarityPressed) {
    // log thread statistics
    zpp_lib::Utils::log_threads_summary();

    ZPP_LOG_DBG("Starting WaitOnButton demo");
    // create the WaitOnButton instance and start it
    multi_tasking::WaitOnButton wait_on_button("ButtonThread");
    auto res = wait_on_button.start();
    if (!res) {
      ZPP_LOG_ERR("Cannot start wait_on_button: %d", static_cast<int>(res.error()));
      return -1;
    }

    // wait that the WaitOnButton thread started
    ZPP_LOG_DBG("Calling wait_started()");
    wait_on_button.wait_started();
    ZPP_LOG_DBG("wait_started() unblocked");

    // log thread statistics
    zpp_lib::Utils::log_threads_summary();

    // wait for the thread to exit (will not because of infinite loop in WaitOnButton)
    wait_on_button.wait_exit();
    // or do busy waiting
    while (true) {
    }
  } else if (button2.read() == zpp_lib::kPolarityPressed) {
    ZPP_LOG_DBG("Starting Clock demo");
    // create and start a clock
    multi_tasking::Clock clock;
    clock.start();
  } else if (button3.read() == zpp_lib::kPolarityPressed) {
    ZPP_LOG_DBG("Starting Deadlock demo");

    // create a first deadlock instance
    multi_tasking::Deadlock deadlock0(0, "Thread0");
    deadlock0.start();

    // create a second deadlock instance
    multi_tasking::Deadlock deadlock1(1, "Thread1");
    deadlock1.start();

    // wait for both threads to terminate (will not because of deadlock)
    deadlock0.wait();
    deadlock1.wait();
  } else {
    ZPP_LOG_DBG("Starting Consumer/Producer demo");

    using BufferType     = Rect;
    using ValueGenerator = RandomRectGenerator;
    multi_tasking::Buffer<BufferType> buffer;
    multi_tasking::Producer<BufferType, ValueGenerator> producer(buffer);
    multi_tasking::Consumer<BufferType> consumer(buffer);

    producer.start();
    consumer.start();

    // wait for threads to terminate (will not)
    producer.wait();
    consumer.wait();
  }

  return 0;
}
