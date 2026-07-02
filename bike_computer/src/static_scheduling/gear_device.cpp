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
 * @file gear_device.cpp
 * @author Serge Ayer <serge.ayer@hefr.ch>
 *
 * @brief GearDevice implementation (static scheduling)
 *
 * @date 2025-07-01
 * @version 1.0.0
 ***************************************************************************/

#include "gear_device.hpp"

// from common
#include "common/task_manager.hpp"

// zpp_lib
#include "zpp_include/time.hpp"

namespace bike_computer::static_scheduling {

GearDevice::GearDevice()
    : _button2(zpp_lib::InterruptIn::PinName::BUTTON2),
      _button3(zpp_lib::InterruptIn::PinName::BUTTON3),
      _button4(zpp_lib::InterruptIn::PinName::BUTTON4) {}

uint8_t GearDevice::get_current_gear() {
  std::chrono::microseconds initial_time = zpp_lib::Time::get_uptime();
  std::chrono::microseconds elapsed_time = std::chrono::microseconds::zero();

  // we bound the change to one decrement/increment per call
  // we increment/decrement rotation speed when button3/button4 is pressed
  // while button2 is pressed
  bool has_changed = false;
  while (elapsed_time < TaskManager::get_task_computation_time(TaskManager::TaskType::GearTaskType)) {
    if (!has_changed) {
      if (_button2.read() == zpp_lib::kPolarityPressed) {
        if (_button3.read() == zpp_lib::kPolarityPressed) {
          if (_current_gear > bike_computer::kMinGear) {
            _current_gear--;
          }
          has_changed = true;
        }

        if (_button4.read() == zpp_lib::kPolarityPressed) {
          if (_current_gear < bike_computer::kMaxGear) {
            _current_gear++;
          }
          has_changed = true;
        }
      }
    }
    elapsed_time = zpp_lib::Time::get_uptime() - initial_time;
  }
  return _current_gear;
}

uint8_t GearDevice::get_current_gear_size() const {
  // simulate task computation by waiting for the required task run time
  // wait_us(kTaskRunTime.count());
  return bike_computer::kMaxGearSize - _current_gear;
}

}  // namespace bike_computer::static_scheduling
