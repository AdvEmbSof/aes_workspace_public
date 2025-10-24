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
#include "zpp_include/utils.hpp"
#include "zpp_include/this_thread.hpp"

// local
#include "wait_on_button.hpp"

LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);

int main(void) {
  using namespace std::literals;

  LOG_DBG("Multi-tasking program started");
  // log thread statistics
  zpp_lib::Utils::logThreadsSummary();
  
  // check which button is pressed
  zpp_lib::InterruptIn<zpp_lib::PinName::BUTTON1> button1;
  if (button1.read() == zpp_lib::kPolarityPressed) {
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
  }

  return 0;
}
