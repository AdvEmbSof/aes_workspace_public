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
#include "zpp_include/interrupt_in.hpp"
#include "zpp_include/this_thread.hpp"
#include "zpp_include/utils.hpp"

// local
#include "memory_fragmenter.hpp"
#include "memory_leak.hpp"

LOG_MODULE_REGISTER(memory_demo, CONFIG_APP_LOG_LEVEL);

int main(void) {
  using namespace std::literals;

  LOG_DBG("Memory demo program started");

  // check which button is pressed
  zpp_lib::InterruptIn<zpp_lib::PinName::BUTTON1> button1;
  zpp_lib::InterruptIn<zpp_lib::PinName::BUTTON2> button2;
  zpp_lib::InterruptIn<zpp_lib::PinName::BUTTON3> button3;
  if (button1.read() == zpp_lib::kPolarityPressed) {
    LOG_DBG("Starting MemoryLeak demo");

    static constexpr uint8_t kNbrOfIterations = 20;
    for (int i = 0; i < kNbrOfIterations; i++) {
      memory_demo::MemoryLeak memoryLeak;
      memoryLeak.use();

      zpp_lib::Utils::logHeapSummary();

      zpp_lib::ThisThread::sleep_for(1s);
    }
  } else if (button2.read() == zpp_lib::kPolarityPressed) {
    memory_demo::MemoryFragmenter memoryFragmenter;
    memoryFragmenter.fragmentMemory();
  }

  return 0;
}
