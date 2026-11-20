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

// zpp_lib
#include "zpp_include/interrupt_in.hpp"
#include "zpp_include/this_thread.hpp"
#include "zpp_include/utils.hpp"
#include "zpp_include/zpp_log.hpp"

// local
#include "memory_fragmenter.hpp"
#include "memory_leak.hpp"
#include "stack_overflow.hpp"

ZPP_LOG_MODULE_REGISTER(memory_demo, CONFIG_APP_LOG_LEVEL);

int main() {
  using std::literals::chrono_literals::operator""s;

  ZPP_LOG_DBG("Memory demo program started");

  // check which button is pressed
  zpp_lib::InterruptIn button1(zpp_lib::InterruptIn::PinName::BUTTON1);
  zpp_lib::InterruptIn button2(zpp_lib::InterruptIn::PinName::BUTTON2);
  zpp_lib::InterruptIn button3(zpp_lib::InterruptIn::PinName::BUTTON3);
  if (button1.read() == zpp_lib::kPolarityPressed) {
    ZPP_LOG_DBG("Starting MemoryLeak demo");

    static constexpr uint8_t kNbrOfIterations = 20;
    for (uint8_t i = 0; i < kNbrOfIterations; i++) {
      memory_demo::MemoryLeak memory_leak;
      memory_leak.use();

      ZPP_LOG_DBG("Heap summary:");
      zpp_lib::Utils::log_heap_summary();

      zpp_lib::ThisThread::sleep_for(1s);
    }
  } else if (button2.read() == zpp_lib::kPolarityPressed) {
    ZPP_LOG_DBG("Starting MemoryFragmenter demo");

    memory_demo::MemoryFragmenter memory_fragmenter;
    memory_fragmenter.fragment_memory();
  } else if (button3.read() == zpp_lib::kPolarityPressed) {
    ZPP_LOG_DBG("Starting StackOverflow demo");

    memory_demo::StackOverflow stack_overflow;
    static constexpr uint8_t kNbrOfIterations = 20;
    for (uint8_t i = 0; i < kNbrOfIterations; i++) {
      stack_overflow.allocate_on_stack();

      ZPP_LOG_DBG("Thread summary: iteration %d", i);
      zpp_lib::Utils::log_threads_summary();

      zpp_lib::ThisThread::sleep_for(1s);
    }
  }

  return 0;
}
