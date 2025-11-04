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
 * @brief Implementation of the Clock class
 *
 * @date 2025-07-01
 * @version 1.0.0
 ***************************************************************************/

#include "clock_with_mutex.hpp"

// zephyr
#include <zephyr/logging/log.h>

// zpp_lib
#include "zpp_include/this_thread.hpp"

LOG_MODULE_DECLARE(multi_tasking, CONFIG_APP_LOG_LEVEL);

namespace multi_tasking {

Clock::Clock()
    : _displayQueue("CDQueue"),
      _displayWork(std::bind(&Clock::displayCurrentTime, this)),
      _updateQueue("TQueue"),
      _updateThread(zpp_lib::PreemptableThreadPriority::PriorityNormal, "TThread"),
      _updateWork(std::bind(&Clock::updateCurrentTime, this)) {}

zpp_lib::ZephyrResult Clock::start() {
  // Start a thread for running the _tickerQueue work queue.
  // Events are dispatched to the queue in the tickerUpdate() method called by the ticker.
  auto res = _updateThread.start(std::bind(&zpp_lib::WorkQueue::run, &_updateQueue));
  if (!res) {
    LOG_ERR("Cannot start ticker thread: %d", (int)res.error());
    return res;
  }

  // Call the updateFromTicker() method every second (from ISR context)
  TickerFunction updateFromTickerFunction = std::bind(&Clock::updateFromTicker, this);
  res = _updateTicker.attach(updateFromTickerFunction, clockUpdateTimeout);
  if (!res) {
    LOG_ERR("Cannot attach update ticker: %d", (int)res.error());
    return res;
  }

  // Call the displayFromTicker() method every second (from ISR context)
  TickerFunction displayFromTickerFunction = std::bind(&Clock::displayFromTicker, this);
  res = _displayTicker.attach(displayFromTickerFunction, clockDisplayTimeout);
  if (!res) {
    LOG_ERR("Cannot attach display ticker: %d", (int)res.error());
    return res;
  }

  // run the displayQueue from the calling thread
  _displayQueue.run();

  // should not get here
  __ASSERT(false, "Should not get here");

  return res;
}

void Clock::displayFromTicker() {
  // this method runs in ISR mode -> we cannot allocate memory or perform other forbidden
  // operations
  auto res = _displayQueue.call(_displayWork);
  __ASSERT(res, "Cannot call display on queue: %d", (int)res.error());
}

void Clock::displayCurrentTime() {
  DateTimeType dt = {0};

  auto res = _mutex.lock();
  __ASSERT(res, "Cannot lock mutex: %d", (int)res.error());

  dt.day  = _currentTime.day;
  dt.hour = _currentTime.hour;

  static constexpr std::chrono::microseconds waitTime = 1000ms;
  zpp_lib::ThisThread::busyWait(waitTime);

  dt.minute = _currentTime.minute;
  dt.second = _currentTime.second;

  res = _mutex.unlock();
  __ASSERT(res, "Cannot unlock mutex: %d", (int)res.error());

  printk("Day %u Hour %u min %u sec %u\n", dt.day, dt.hour, dt.minute, dt.second);
}

void Clock::updateFromTicker() {
  // this method runs in ISR mode -> we cannot allocate memory or perform other forbidden
  // operations
  auto res = _updateQueue.call(_updateWork);
  __ASSERT(res, "Cannot call update on queue: %d", (int)res.error());
}

void Clock::updateCurrentTime() {
  auto res = _mutex.lock();
  __ASSERT(res, "Cannot lock mutex: %d", (int)res.error());

  _currentTime.second +=
      std::chrono::duration_cast<std::chrono::seconds>(clockUpdateTimeout).count();

  if (_currentTime.second > 59) {
    _currentTime.second = 0;
    _currentTime.minute++;
    if (_currentTime.minute > 59) {
      _currentTime.minute = 0;
      _currentTime.hour++;
      if (_currentTime.hour > 23) {
        _currentTime.hour = 0;
        _currentTime.day++;
      }
    }
  }

  res = _mutex.unlock();
  __ASSERT(res, "Cannot unlock mutex: %d", (int)res.error());
}

}  // namespace multi_tasking
