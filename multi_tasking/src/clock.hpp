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
 * @file clock.hpp
 * @author Serge Ayer <serge.ayer@hefr.ch>
 *
 * @brief Declaration of the Clock class
 *
 * @date 2025-07-01
 * @version 1.0.0
 ***************************************************************************/

#pragma once

// stl
#include <chrono>
#include <functional>

// zpp_lib
#include "zpp_include/thread.hpp"
#include "zpp_include/ticker.hpp"
#include "zpp_include/work_queue.hpp"

namespace multi_tasking {

using std::literals::chrono_literals::operator""ms;

class Clock {
public:
  struct DateTimeType {
    uint32_t day;
    uint32_t hour;
    uint32_t minute;
    uint32_t second;
  };

  Clock();

  // method called for starting the clock demo
  [[nodiscard]] zpp_lib::ZephyrResult start();

private:
  void display_from_ticker();
  void display_current_time();
  void update_from_ticker();
  void update_current_time();

  // type definition used by tickers and queues
  using TickerFunction    = std::function<void()>;
  using WorkQueueFunction = std::function<void()>;
  // used for display the current time
  zpp_lib::WorkQueue _display_queue;
  zpp_lib::Ticker<TickerFunction> _display_ticker;
  zpp_lib::Work<Clock> _display_work;
  // used for updating _currentTime
  zpp_lib::WorkQueue _update_queue;
  zpp_lib::Thread _update_thread;
  zpp_lib::Ticker<TickerFunction> _update_ticker;
  zpp_lib::Work<Clock> _update_work;
#if CONFIG_CURRENT_TIME_MUTEX
  zpp_lib::Mutex _mutex;
#endif  // CONFIG_CURRENT_TIME_MUTEX

  static constexpr auto kNbrOfSecondsInMinute = 60;
  static constexpr auto kNbrOfMinutesInHour   = 60;
  static constexpr auto kNbrOfHoursInDay      = 24;
  static constexpr auto kInitialHour          = 10;
  DateTimeType _current_time{.day = 0, .hour = kInitialHour, .minute = kNbrOfMinutesInHour - 1, .second = kNbrOfSecondsInMinute - 2};
  static constexpr std::chrono::milliseconds kClockUpdateTimeout  = 1000ms;
  static constexpr std::chrono::milliseconds kClockDisplayTimeout = 1000ms;
};

}  // namespace multi_tasking
