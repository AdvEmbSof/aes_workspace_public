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
 * @file deadlock.cpp
 * @author Serge Ayer <serge.ayer@hefr.ch>
 *
 * @brief Implementation of the Deadlock class
 *
 * @date 2025-07-01
 * @version 1.0.0
 ***************************************************************************/

#include "deadlock.hpp"

// zephyr

// zpp_lib
#include "zpp_include/zpp_assert.hpp"
#include "zpp_include/zpp_log.hpp"

ZPP_LOG_MODULE_DECLARE(multi_tasking, CONFIG_APP_LOG_LEVEL);

namespace multi_tasking {

Deadlock::Deadlock(uint8_t index, const char* threadName)
    : c_index(index), _thread(zpp_lib::PreemptableThreadPriority::PriorityNormal, threadName) {
  ZPP_ASSERT(index < kNbrOfMutexes, "Index must be smaller than %d", kNbrOfMutexes);
}

void Deadlock::start() {
  auto res = _thread.start([this]() { this->execute(); });
  ZPP_ASSERT(res, "Cannot start deadlock thread: %d", (int)res.error());
}

void Deadlock::wait() {
  auto res = _thread.join();
  ZPP_ASSERT(res, "Cannot join deadlock thread: %d", (int)res.error());
}

// Complexity is increased by the use of Zephyr macros
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void Deadlock::execute() const {
  // _index is initialized in the constructor and asserted for correctness
  // NOLINTBEGIN(cppcoreguidelines-pro-bounds-constant-array-index)
  // enter the first critical section
  auto res = s_mutex[c_index].lock();
  ZPP_ASSERT(res, "Cannot lock mutex: %d", (int)res.error());
  ZPP_LOG_DBG("Thread %d entered critical section %d", c_index, c_index);

  // perform some operations
  zpp_lib::ThisThread::busy_wait(kProcessingWaitTime);
  ZPP_LOG_DBG("Thread %d processing in mutex %d done", c_index, c_index);

  // enter the second critical section
  int second_index = (c_index + 1) % kNbrOfMutexes;
  ZPP_LOG_DBG("Thread %d trying to enter critical section %d", c_index, second_index);
  res = s_mutex[second_index].lock();
  ZPP_ASSERT(res, "Cannot lock mutex: %d", (int)res.error());
  ZPP_LOG_DBG("Thread %d entered critical section %d", c_index, second_index);

  // perform some operations
  zpp_lib::ThisThread::busy_wait(kProcessingWaitTime);
  ZPP_LOG_DBG("Thread %d processing in mutex %d and %d done", c_index, c_index, second_index);

  // exit the second critical section
  res = s_mutex[second_index].unlock();
  ZPP_ASSERT(res, "Cannot unlock mutex: %d", (int)res.error());

  // perform some operations
  zpp_lib::ThisThread::busy_wait(kProcessingWaitTime);
  ZPP_LOG_DBG("Thread %d processing in mutex %d done", c_index, c_index);

  // exit the first critical section
  res = s_mutex[c_index].unlock();
  ZPP_ASSERT(res, "Cannot unlock mutex: %d", (int)res.error());
  // NOLINTEND(cppcoreguidelines-pro-bounds-constant-array-index)
}

}  // namespace multi_tasking
