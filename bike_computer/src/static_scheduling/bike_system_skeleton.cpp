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
 * @file bike_system.cpp
 * @author Serge Ayer <serge.ayer@hefr.ch>
 *
 * @brief Bike System implementation (static scheduling)
 *
 * @date 2025-07-01
 * @version 1.0.0
 ***************************************************************************/

#include "bike_system.hpp"

// std
#include <chrono>

// zephyr
// false positive cpplint warning
// NOLINTNEXTLINE(build/include_order)
#include <zephyr/logging/log.h>

// zpp_lib
#include "zpp_include/this_thread.hpp"
#include "zpp_include/time.hpp"
#include "zpp_include/work_queue.hpp"

LOG_MODULE_DECLARE(bike_computer, CONFIG_APP_LOG_LEVEL);

namespace bike_computer {

namespace static_scheduling {

zpp_lib::ZephyrResult BikeSystem::start() {
  LOG_INF("Starting Super-Loop without event handling");

  auto res = initialize();
  if (!res) {
    LOG_ERR("Init failed: %d", (int)res.error());
    return res;
  }

  LOG_DBG("Starting super-loop");

  // initialize the task manager phase
  _taskManager.initializePhase();

  uint32_t iteration                                 = 0;
  static constexpr uint32_t iterationsForFixingDrift = 10;
  while (true) {
    auto startTime = zpp_lib::Time::getUpTime();

    // TODO: implement calls to different tasks based on computed schedule

    // register the time at the end of the cyclic schedule period and print the
    // elapsed time for the period
    std::chrono::microseconds endTime = zpp_lib::Time::getUpTime();
    const auto cycle =
        std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    LOG_DBG("Repeating cycle time is %" PRIu64 " milliseconds", cycle.count());

    if (atomic_test_bit(&_stopFlag, 1)) {
      break;
    }

    // fix the schedule drift to pass the tests
    // this demonstrates that static scheduling is very sensitive to overload
    if (iteration % iterationsForFixingDrift == 0) {
      _taskManager.initializePhase();
    }
  }

  return res;
}

void BikeSystem::stop() {
  atomic_set_bit(&_stopFlag, 1);
  if (gTTCE.isStarted()) {
    gTTCE.stop();
  }
}

zpp_lib::ZephyrResult BikeSystem::initialize() {
  // initialize the display
  auto res = _bikeDisplay.initialize();
  if (!res) {
    LOG_ERR("Cannot initialize display: %d", (int)res.error());
    return res;
  }

  // initialize the sensor device
  res = _sensorDevice.initialize();
  if (!res) {
    LOG_ERR("Sensor not present or initialization failed: %d", (int)res.error());
  }

  return zpp_lib::ZephyrResult();
}

void BikeSystem::gearTask() {
  // gear task
  _taskManager.registerTaskStart(TaskManager::TaskType::GearTaskType);

  // no need to protect access to data members (single threaded)
  _currentGear     = _gearDevice.getCurrentGear();
  _currentGearSize = _gearDevice.getCurrentGearSize();

  _taskManager.simulateComputationTime(TaskManager::TaskType::GearTaskType);
}

void BikeSystem::speedDistanceTask() {
  // speed and distance task
  _taskManager.registerTaskStart(TaskManager::TaskType::SpeedTaskType);

  const auto pedalRotationTime = _pedalDevice.getCurrentRotationTime();
  _speedometer.setCurrentRotationTime(pedalRotationTime);
  _speedometer.setGearSize(_currentGearSize);
  // no need to protect access to data members (single threaded)
  _currentSpeed     = _speedometer.getCurrentSpeed();
  _traveledDistance = _speedometer.getDistance();

  _taskManager.simulateComputationTime(TaskManager::TaskType::SpeedTaskType);
}

void BikeSystem::temperatureTask() {
  _taskManager.registerTaskStart(TaskManager::TaskType::TemperatureTaskType);

  // TO DO: read temperature from _sensorDevice
  
  // simulate task computation by waiting for the required task computation time
  _taskManager.simulateComputationTime(TaskManager::TaskType::TemperatureTaskType);
}

void BikeSystem::resetTask() {
  _taskManager.registerTaskStart(TaskManager::TaskType::ResetTaskType);

  if (_resetDevice.checkReset()) {
    std::chrono::microseconds responseTime =
        zpp_lib::Time::getUpTime() - _resetDevice.getPressTime();
    LOG_INF("Reset task: response time is %" PRIu64 " usecs", responseTime.count());
    _speedometer.reset();
  }

  _taskManager.simulateComputationTime(TaskManager::TaskType::ResetTaskType);
}

void BikeSystem::displayTask1() {
  _taskManager.registerTaskStart(TaskManager::TaskType::DisplayTask1Type);

  // TODO: update gear, speed and distance displayed on screen
  
  _taskManager.simulateComputationTime(TaskManager::TaskType::DisplayTask1Type);
}

void BikeSystem::displayTask2() {
  _taskManager.registerTaskStart(TaskManager::TaskType::DisplayTask2Type);

  // TODO: update temperature on screen
  
  _taskManager.simulateComputationTime(TaskManager::TaskType::DisplayTask2Type);
}

}  // namespace static_scheduling

}  // namespace bike_computer
