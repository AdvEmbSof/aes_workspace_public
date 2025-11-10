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
#include <zephyr/logging/log.h>

// zpp_lib
#include "zpp_include/this_thread.hpp"
#include "zpp_include/utils.hpp"

// local
#include "buffer_solution.hpp"
#include "clock_with_mutex.hpp"
#include "consumer.hpp"
#include "deadlock.hpp"
#include "producer.hpp"
#include "wait_on_button.hpp"

LOG_MODULE_REGISTER(multi_tasking, CONFIG_APP_LOG_LEVEL);

class RandomIntGenerator {
 public:
  static constexpr uint8_t kMaxRandomValue = 20;

  static uint32_t produceNextValue() { return sys_rand32_get() % kMaxRandomValue; }
};

class RandomDoubleGenerator {
 public:
  static constexpr double randomValues[] = {1.1, 2.2, 3.3, 4.4, 5.5};
  static double produceNextValue() {
    return randomValues[sys_rand32_get() %
                        (sizeof(randomValues) / sizeof(randomValues[0]))];
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
  static constexpr Rect randomValues[] = {{1, 1}, {2, 2}, {3, 3}, {4, 4}, {5, 5}};
  static Rect produceNextValue() {
    return randomValues[sys_rand32_get() %
                        (sizeof(randomValues) / sizeof(randomValues[0]))];
  }
};

int main(void) {
  using namespace std::literals;

  LOG_DBG("Multi-tasking program started");

  // check which button is pressed
  zpp_lib::InterruptIn<zpp_lib::PinName::BUTTON1> button1;
  zpp_lib::InterruptIn<zpp_lib::PinName::BUTTON2> button2;
  zpp_lib::InterruptIn<zpp_lib::PinName::BUTTON3> button3;
  if (button1.read() == zpp_lib::kPolarityPressed) {
    // log thread statistics
    zpp_lib::Utils::logThreadsSummary();

    LOG_DBG("Starting WaitOnButton demo");
    // create the WaitOnButton instance and start it
    multi_tasking::WaitOnButton waitOnButton("ButtonThread");
    auto res = waitOnButton.start();
    if (!res) {
      LOG_ERR("Cannot start waitOnButton: %d", static_cast<int>(res.error()));
      return -1;
    }

    // wait that the WaitOnButton thread started
    LOG_DBG("Calling wait_started()");
    waitOnButton.wait_started();
    LOG_DBG("wait_started() unblocked");

    // log thread statistics
    zpp_lib::Utils::logThreadsSummary();

    // wait for the thread to exit (will not because of infinite loop in WaitOnButton)
    // waitOnButton.wait_exit();
    // or do busy waiting
    while (true) {
    }
  } else if (button2.read() == zpp_lib::kPolarityPressed) {
    LOG_DBG("Starting Clock demo");
    // create and start a clock
    multi_tasking::Clock clock;
    clock.start();
  } else if (button3.read() == zpp_lib::kPolarityPressed) {
    LOG_DBG("Starting Deadlock demo");

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
    LOG_DBG("Starting Consumer/Producer demo");

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
