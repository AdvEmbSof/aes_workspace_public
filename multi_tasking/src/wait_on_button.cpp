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
 * @file wait_on_button.cpp
 * @author Serge Ayer <serge.ayer@hefr.ch>
 *
 * @brief Implementation of the WaitOnButton class
 *
 * @date 2025-07-01
 * @version 1.0.0
 ***************************************************************************/
#include "wait_on_button.hpp"

// zephyr

// zpp_lib
#include "zpp_include/time.hpp"
#include "zpp_include/zpp_log.hpp"

ZPP_LOG_MODULE_DECLARE(multi_tasking, CONFIG_APP_LOG_LEVEL);

namespace multi_tasking {

WaitOnButton::WaitOnButton(const char* threadName)
    : _thread(zpp_lib::PreemptableThreadPriority::PriorityNormal, threadName), _pressed_time(std::chrono::microseconds::zero()),
      _push_button_token(_push_button.add_callback([this]() { this->on_button_pressed(); })) {
  ZPP_LOG_DBG("WaitOnButton initialized");
}

zpp_lib::ZephyrResult WaitOnButton::start() {
  auto res = _thread.start([this]() { this->wait_for_button_event(); });
  if (!res) {
    ZPP_LOG_ERR("Failed to start thread: %d", (int)res.error());
    return res;
  }
  ZPP_LOG_DBG("Thread started successfully");
  return res;
}

void WaitOnButton::wait_started() {
  _event.wait_any(kStartedEvent);
}

void WaitOnButton::wait_exit() {
  auto res = _thread.join();
  if (!res) {
    ZPP_LOG_ERR("join() failed: %d", (int)res.error());
  }
}

// Complexity is increased by the use of Zephyr macros
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void WaitOnButton::wait_for_button_event() {
  ZPP_LOG_DBG("Waiting for button press");
  _event.set(kStartedEvent);

  while (true) {
    _event.wait_any(kPressedEvent);
    std::chrono::microseconds time    = zpp_lib::Time::get_uptime();
    std::chrono::microseconds latency = time - _pressed_time;
    ZPP_LOG_DBG("Button pressed with response time: %lld usecs", latency.count());
    ZPP_LOG_DBG("Waiting for button press");
  }
}

void WaitOnButton::on_button_pressed() {
  _pressed_time = zpp_lib::Time::get_uptime();
  _event.set(kPressedEvent);
}

}  // namespace multi_tasking
