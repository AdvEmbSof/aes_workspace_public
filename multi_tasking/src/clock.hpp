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
 * @brief Declaration of the Clock class
 *
 * @date 2025-07-01
 * @version 1.0.0
 ***************************************************************************/

#pragma once

// stl
#include <functional>

// zpp_lib
#include "zpp_include/thread.hpp"
#include "zpp_include/ticker.hpp"
#include "zpp_include/work_queue.hpp"

namespace multi_tasking {

using namespace std::literals;

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
  zpp_lib::ZephyrResult start();

 private:
  void displayFromTicker();
  void displayCurrentTime();
  void updateFromTicker();
  void updateCurrentTime();

  // type definition used by tickers and queues
  using TickerFunction = std::function<void()>;
  using WorkQueueFunction = std::function<void()>;  
  // used for display the current time
  zpp_lib::WorkQueue _displayQueue;
  zpp_lib::Ticker<TickerFunction> _displayTicker;
  zpp_lib::Work<WorkQueueFunction> _displayWork;  
  // used for updating _currentTime
  zpp_lib::WorkQueue _updateQueue;
  zpp_lib::Thread _updateThread;
  zpp_lib::Ticker<TickerFunction> _updateTicker;
  zpp_lib::Work<WorkQueueFunction> _updateWork;
  DateTimeType _currentTime{.day = 0, .hour = 10, .minute = 59, .second = 58};
  static constexpr std::chrono::milliseconds clockUpdateTimeout  = 1000ms;
  static constexpr std::chrono::milliseconds clockDisplayTimeout = 1000ms;
};

}  // namespace multi_tasking
