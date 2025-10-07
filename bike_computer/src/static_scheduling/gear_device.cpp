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

namespace bike_computer {

namespace static_scheduling {

uint8_t GearDevice::getCurrentGear() {
  std::chrono::microseconds initialTime = zpp_lib::Time::getUpTime();
  std::chrono::microseconds elapsedTime = std::chrono::microseconds::zero();

  // we bound the change to one decrement/increment per call
  // we increment/decrement rotation speed when button3/button4 is pressed
  // while button2 is pressed
  bool hasChanged = false;
  while (elapsedTime <
         TaskManager::getTaskComputationTime(TaskManager::TaskType::GearTaskType)) {
    if (!hasChanged) {
      if (_button2.read() == zpp_lib::kPolarityPressed) {
        if (_button3.read() == zpp_lib::kPolarityPressed) {
          _currentGear--;
          hasChanged = true;
        }

        if (_button4.read() == zpp_lib::kPolarityPressed) {
          _currentGear++;
          hasChanged = true;
        }
      }
    }
    elapsedTime = zpp_lib::Time::getUpTime() - initialTime;
  }
  return _currentGear;
}

uint8_t GearDevice::getCurrentGearSize() const {
  // simulate task computation by waiting for the required task run time
  // wait_us(kTaskRunTime.count());
  return bike_computer::kMaxGearSize - _currentGear;
}

}  // namespace static_scheduling

}  // namespace bike_computer
