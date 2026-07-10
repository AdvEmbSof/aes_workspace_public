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

// zpp_lib
#include "zpp_include/this_thread.hpp"
#include "zpp_include/zpp_assert.hpp"
#include "zpp_include/zpp_log.hpp"

ZPP_LOG_MODULE_DECLARE(multi_tasking, CONFIG_APP_LOG_LEVEL);

namespace multi_tasking {

Clock::Clock()
    : _display_queue("CDQueue"), _display_work(zpp_lib::Work<Clock>(this, &Clock::display_current_time)), _update_queue("TQueue"),
      _update_thread(zpp_lib::PreemptableThreadPriority::PriorityNormal, "TThread"), _update_work(zpp_lib::Work<Clock>(this, &Clock::update_current_time)) {}

// Complexity is increased by the use of Zephyr macros
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
zpp_lib::ZephyrResult Clock::start() {
  // Start a thread for running the _tickerQueue work queue.
  // Events are dispatched to the queue in the tickerUpdate() method called by the
  // ticker.
  auto res = _update_thread.start([this] { _update_queue.run(); });
  if (!res) {
    ZPP_LOG_ERR("Cannot start ticker thread: %d", (int)res.error());
    return res;
  }

  // Call the updateFromTicker() method every second (from ISR context)
  TickerFunction update_from_ticker_function = [this] { update_from_ticker(); };
  res                                        = _update_ticker.attach(update_from_ticker_function, kClockUpdateTimeout);
  if (!res) {
    ZPP_LOG_ERR("Cannot attach update ticker: %d", (int)res.error());
    return res;
  }

  // Call the displayFromTicker() method every second (from ISR context)
  TickerFunction display_from_ticker_function = [this] { display_from_ticker(); };
  res                                         = _display_ticker.attach(display_from_ticker_function, kClockDisplayTimeout);
  if (!res) {
    ZPP_LOG_ERR("Cannot attach display ticker: %d", (int)res.error());
    return res;
  }

  // run the displayQueue from the calling thread
  _display_queue.run();

  // should not get here
  ZPP_ASSERT(false, "Should not get here");

  return res;
}

void Clock::display_from_ticker() {
  // this method runs in ISR mode -> we cannot allocate memory or perform other
  // forbidden operations
  auto res = _display_queue.call(_display_work);
  ZPP_ASSERT(res, "Cannot call display on queue: %d", (int)res.error());
}

// display_current_time is used as work handler and $
// must be non-const because the work handler is non-const
// NOLINTNEXTLINE(readability-make-member-function-const)
void Clock::display_current_time() {
  DateTimeType dt = {};

  auto res = _mutex.lock();
  ZPP_ASSERT(res, "Cannot lock mutex: %d", (int)res.error());

  dt.day  = _current_time.day;
  dt.hour = _current_time.hour;

  static constexpr std::chrono::microseconds kWaitTime = 1000ms;
  zpp_lib::ThisThread::busy_wait(kWaitTime);

  dt.minute = _current_time.minute;
  dt.second = _current_time.second;

  res = _mutex.unlock();
  ZPP_ASSERT(res, "Cannot unlock mutex: %d", (int)res.error());

  printk("Day %u Hour %u min %u sec %u\n", dt.day, dt.hour, dt.minute, dt.second);
}

void Clock::update_from_ticker() {
  // this method runs in ISR mode -> we cannot allocate memory or perform other
  // forbidden operations updateCurrentTime();
  auto res = _update_queue.call(_update_work);
  ZPP_ASSERT(res, "Cannot call update on queue: %d", (int)res.error());
}

void Clock::update_current_time() {
  auto res = _mutex.lock();
  ZPP_ASSERT(res, "Cannot lock mutex: %d", (int)res.error());

  _current_time.second += std::chrono::duration_cast<std::chrono::seconds>(kClockUpdateTimeout).count();

  if (_current_time.second >= kNbrOfSecondsInMinute) {
    _current_time.second = 0;
    _current_time.minute++;
    if (_current_time.minute >= kNbrOfMinutesInHour) {
      _current_time.minute = 0;
      _current_time.hour++;
      if (_current_time.hour >= kNbrOfHoursInDay) {
        _current_time.hour = 0;
        _current_time.day++;
      }
    }
  }

  res = _mutex.unlock();
  ZPP_ASSERT(res, "Cannot unlock mutex: %d", (int)res.error());
}

}  // namespace multi_tasking
