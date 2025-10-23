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
 * @brief Implementation of the WaitOnButton class
 *
 * @date 2025-07-01
 * @version 1.0.0
 ***************************************************************************/
#include "wait_on_button.hpp"

// zephyr
#include <zephyr/logging/log.h>

// zpp_lib
#include "zpp_include/time.hpp"

LOG_MODULE_REGISTER(wait_on_button, CONFIG_APP_LOG_LEVEL);

namespace multi_tasking {

WaitOnButton::WaitOnButton(const char* threadName)
    : _thread(zpp_lib::PreemptableThreadPriority::PriorityNormal, threadName),
      _pressedTime(std::chrono::microseconds::zero()) {
  _pushButton.fall(std::bind(&WaitOnButton::buttonPressed, this));
  LOG_DBG("WaitOnButton initialized");
}

zpp_lib::ZephyrResult WaitOnButton::start() {
  auto res = _thread.start(std::bind(&WaitOnButton::waitForButtonEvent, this));
  if (!res) {
    LOG_ERR("Failed to start thread: %d", (int)res.error());
    return res;
  }
  LOG_DBG("Thread started successfully");
  return res;
}

void WaitOnButton::wait_started() { _events.wait_any(kStartedEvent); }

void WaitOnButton::wait_exit() {
  auto res = _thread.join();
  if (!res) {
    LOG_ERR("join() failed: %d", (int)res.error());
  }
}

void WaitOnButton::waitForButtonEvent() {
  LOG_DBG("Waiting for button press");
  _events.set(kStartedEvent);

  while (true) {
    _events.wait_any(kPressedEvent);
    std::chrono::microseconds time    = zpp_lib::Time::getUpTime();
    std::chrono::microseconds latency = time - _pressedTime;
    LOG_DBG("Button pressed with response time: %lld usecs", latency.count());
    LOG_DBG("Waiting for button press");
  }
}

void WaitOnButton::buttonPressed() {
  _pressedTime = zpp_lib::Time::getUpTime();
  _events.set(kPressedEvent);
}

}  // namespace multi_tasking
