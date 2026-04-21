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
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(multi_tasking, CONFIG_APP_LOG_LEVEL);

namespace multi_tasking {

// static data member allocation
zpp_lib::Mutex Deadlock::_mutex[kNbrOfMutexes];

Deadlock::Deadlock(int index, const char* threadName)
    : _index(index), _thread(zpp_lib::PreemptableThreadPriority::PriorityNormal, threadName) {}

void Deadlock::start() {
  auto res = _thread.start(std::bind(&Deadlock::execute, this));
  __ASSERT(res, "Cannot start deadlock thread: %d", (int)res.error());
}

void Deadlock::wait() {
  auto res = _thread.join();
  __ASSERT(res, "Cannot join deadlock thread: %d", (int)res.error());
}

void Deadlock::execute() {
  // enter the first critical section
  auto res = _mutex[_index].lock();
  __ASSERT(res, "Cannot lock mutex: %d", (int)res.error());
  LOG_DBG("Thread %d entered critical section %d", _index, _index);

  // perform some operations
  zpp_lib::ThisThread::busyWait(kProcessingWaitTime);
  LOG_DBG("Thread %d processing in mutex %d done", _index, _index);

  // enter the second critical section
  int secondIndex = (_index + 1) % kNbrOfMutexes;
  LOG_DBG("Thread %d trying to enter critical section %d", _index, secondIndex);
  res = _mutex[secondIndex].lock();
  __ASSERT(res, "Cannot lock mutex: %d", (int)res.error());
  LOG_DBG("Thread %d entered critical section %d", _index, secondIndex);

  // perform some operations
  zpp_lib::ThisThread::busyWait(kProcessingWaitTime);
  LOG_DBG("Thread %d processing in mutex %d and %d done", _index, _index, secondIndex);

  // exit the second critical section
  res = _mutex[secondIndex].unlock();
  __ASSERT(res, "Cannot unlock mutex: %d", (int)res.error());

  // perform some operations
  zpp_lib::ThisThread::busyWait(kProcessingWaitTime);
  LOG_DBG("Thread %d processing in mutex %d done", _index, _index);

  // exit the first critical section
  res = _mutex[_index].unlock();
  __ASSERT(res, "Cannot unlock mutex: %d", (int)res.error());
}

}  // namespace multi_tasking
