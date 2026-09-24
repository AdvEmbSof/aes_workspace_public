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
 * @file clock.cpp
 * @author Serge Ayer <serge.ayer@hefr.ch>
 *
 * @brief Implementation of the Clock class
 *
 * @date 2025-07-01
 * @version 1.0.0
 ***************************************************************************/

#include "clock.hpp"

// zephyr

// zpp_lib
#include "zpp_include/this_thread.hpp"
#include "zpp_include/utils.hpp"
#include "zpp_include/zpp_assert.hpp"
#include "zpp_include/zpp_log.hpp"

ZPP_LOG_MODULE_DECLARE(multi_tasking, CONFIG_APP_LOG_LEVEL);

namespace multi_tasking {

Clock::Clock()
    : _display_queue("CDQueue"), _display_work(zpp_lib::Work<Clock>(this, &Clock::display_current_time)), _update_queue("TQueue"),
      _update_thread(zpp_lib::PreemptableThreadPriority::PriorityNormal, "TThread"),
      _update_work(zpp_lib::Work<Clock>(this, &Clock::update_current_time)) {}

zpp_lib::ZephyrResult Clock::start() {
  // Start a thread to run the _ticker_queue work queue.
  // Events are dispatched to the queue in the tickerUpdate() method called by the
  // ticker.
  auto res = _update_thread.start([this] { _update_queue.run(); });
  if (!res) {
    ZPP_LOG_ERR("Cannot start ticker thread: %d", static_cast<int>(res.error()));
    return res;
  }

  // Call the update_from_ticker() method every kClockUpdateTimeout (from ISR context)
  TickerFunction update_from_ticker_function = [this] { update_from_ticker(); };
  res                                        = _update_ticker.attach(update_from_ticker_function, kClockUpdateTimeout);
  if (!res) {
    ZPP_LOG_ERR("Cannot attach update ticker: %d", static_cast<int>(res.error()));
    return res;
  }

  // Call the display_from_ticker() method every kClockDisplayTimeout (from ISR context)
  TickerFunction display_from_ticker_function = [this] { display_from_ticker(); };
  res                                         = _display_ticker.attach(display_from_ticker_function, kClockDisplayTimeout);
  if (!res) {
    ZPP_LOG_ERR("Cannot attach display ticker: %d", static_cast<int>(res.error()));
    return res;
  }

  // log thread statistics
  zpp_lib::Utils::log_threads_summary();

  // run the _display_queue from the calling thread
  _display_queue.run();

  // should not get here
  ZPP_ASSERT(false, "Should not get here");

  return res;
}

// display_from_ticker is used as a callback function for the ticker
// NOLINTNEXTLINE(readability-make-member-function-const)
void Clock::display_from_ticker() {
  // this method runs in ISR mode -> we cannot allocate memory or perform other
  // forbidden operations
  auto res = _display_queue.call(_display_work);
  ZPP_ASSERT(res, "Cannot call display on queue: %d", static_cast<int>(res.error()));
}

// display_current_time is used as work handler and $
// must be non-const because the work handler is non-const
// NOLINTNEXTLINE(readability-make-member-function-const)
void Clock::display_current_time() {
  DateTimeType dt = {};

#if CONFIG_CURRENT_TIME_MUTEX
  auto res = _mutex.lock();
  ZPP_ASSERT(res, "Cannot lock mutex: %d", static_cast<int>(res.error()));
#endif  // CONFIG_CURRENT_TIME_MUTEX

  dt.day  = _current_time.day;
  dt.hour = _current_time.hour;

#if CONFIG_DISPLAY_CURRENT_TIME_WAIT
  using std::literals::chrono_literals::operator""s;
  zpp_lib::ThisThread::sleep_for(1s);
#endif  // CONFIG_DISPLAY_CURRENT_TIME_WAIT

  dt.minute = _current_time.minute;
  dt.second = _current_time.second;

#if CONFIG_CURRENT_TIME_MUTEX
  res = _mutex.unlock();
  ZPP_ASSERT(res, "Cannot unlock mutex: %d", static_cast<int>(res.error()));
#endif  // CONFIG_CURRENT_TIME_MUTEX

  printk("Day %u Hour %u min %u sec %u\n", dt.day, dt.hour, dt.minute, dt.second);
}

// update_from_ticker is used as a callback function for the ticker
// NOLINTNEXTLINE(readability-make-member-function-const)
void Clock::update_from_ticker() {
  // this method runs in ISR mode -> we cannot allocate memory or perform other
  // forbidden operations
  auto res = _update_queue.call(_update_work);
  ZPP_ASSERT(res, "Cannot call update on queue: %d", static_cast<int>(res.error()));
}

void Clock::update_current_time() {

#if CONFIG_CURRENT_TIME_MUTEX
  auto res = _mutex.lock();
  ZPP_ASSERT(res, "Cannot lock mutex: %d", static_cast<int>(res.error()));
#endif  // CONFIG_CURRENT_TIME_MUTEX

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

#if CONFIG_CURRENT_TIME_MUTEX
  res = _mutex.unlock();
  ZPP_ASSERT(res, "Cannot unlock mutex: %d", static_cast<int>(res.error()));
#endif  // CONFIG_CURRENT_TIME_MUTEX
}

}  // namespace multi_tasking
