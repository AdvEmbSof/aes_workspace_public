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
 * @file deadlock.hpp
 * @author Serge Ayer <serge.ayer@hefr.ch>
 *
 * @brief Declaration of the Deadlock class
 *
 * @date 2025-07-01
 * @version 1.0.0
 ***************************************************************************/

#pragma once

// stl
#include <chrono>

// zpp_lib
#include "zpp_include/mutex.hpp"
#include "zpp_include/this_thread.hpp"
#include "zpp_include/thread.hpp"

namespace multi_tasking {

using std::literals::chrono_literals::operator""us;

class Deadlock {
 public:
  Deadlock(int index, const char* threadName);

  void start();
  void wait();

 private:
  void execute();

  // time that the threads should spend processing (e.g. wait in our case)
  static constexpr std::chrono::microseconds kProcessingWaitTime = 1000000us;
  static constexpr int kNbrOfMutexes                             = 2;
  uint8_t _index;
  zpp_lib::Thread _thread;
  // the mutex must be declared as static for being a class instance
  static zpp_lib::Mutex _mutex[kNbrOfMutexes];
};

}  // namespace multi_tasking
