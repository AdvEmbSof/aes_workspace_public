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
 * @file buffer.hpp
 * @author Serge Ayer <serge.ayer@hefr.ch>
 *
 * @brief Declaration/Implementation of the Buffer class
 *
 * @date 2025-07-01
 * @version 1.0.0
 ***************************************************************************/

#pragma once

// zephyr
#include <zephyr/random/random.h>

// zpp_lib
#include "zpp_include/digital_out.hpp"
#include "zpp_include/mutex.hpp"
#include "zpp_include/semaphore.hpp"
#include "zpp_include/this_thread.hpp"

namespace multi_tasking {

using std::literals::chrono_literals::operator""ms;

static constexpr uint8_t kLedOff = 0;
static constexpr uint8_t kLedOn  = 1;

template <typename T>
class Buffer {
 public:
  Buffer()
      : _producerLed(zpp_lib::DigitalOut::PinName::LED0, kLedOff),
        _consumerLed(zpp_lib::DigitalOut::PinName::LED1, kLedOff) {}

  uint32_t append(const T& data) {
    // make sure that we can produce without overflow
    auto res = _inSemaphore.acquire();
    __ASSERT(res, "Cannot acquire inSemaphore: %d", (int)res.error());

    // lock buffer
    res = _producerConsumerMutex.lock();
    __ASSERT(res, "Cannot lock mutex: %d", (int)res.error());

    _producerLed            = kLedOn;
    _buffer[_producerIndex] = data;
    uint32_t index          = _producerIndex;
    _producerIndex          = (_producerIndex + 1) % kBufferSize;

    zpp_lib::ThisThread::busyWait(computeRandomWaitTime(kApppendWaitTime));
    _producerLed = kLedOff;

    // unlock buffer
    res = _producerConsumerMutex.unlock();
    __ASSERT(res, "Cannot unlock mutex: %d", (int)res.error());

    // tell that one element is available for consumer
    res = _outSemaphore.release();
    __ASSERT(res, "Cannot release outSemaphore: %d", (int)res.error());

    return index;
  }

  uint32_t extract(T& data) {
    // make sure that we can consume without underflow
    auto res = _outSemaphore.acquire();
    __ASSERT(res, "Cannot acquire outSemaphore: %d", (int)res.error());

    // lock buffer
    res = _producerConsumerMutex.lock();
    __ASSERT(res, "Cannot lock mutex: %d", (int)res.error());

    _consumerLed   = kLedOn;
    data           = _buffer[_consumerIndex];
    uint32_t index = _consumerIndex;
    _consumerIndex = (_consumerIndex + 1) % kBufferSize;

    zpp_lib::ThisThread::busyWait(computeRandomWaitTime(kExtractWaitTime));
    _consumerLed = kLedOff;

    // unlock buffer
    res = _producerConsumerMutex.unlock();
    __ASSERT(res, "Cannot unlock mutex: %d", (int)res.error());

    // tell that one element is available for producer
    res = _inSemaphore.release();
    __ASSERT(res, "Cannot release inSemaphore: %d", (int)res.error());

    return index;
  }

  std::chrono::milliseconds computeRandomWaitTime(
      const std::chrono::milliseconds& waitTime) {
    return std::chrono::milliseconds((sys_rand32_get() % waitTime.count()) +
                                     waitTime.count());
  }

 private:
  static constexpr std::chrono::milliseconds kApppendWaitTime = 500ms;
  static constexpr std::chrono::milliseconds kExtractWaitTime = 500ms;
  static constexpr uint8_t kBufferSize                        = 10;
  zpp_lib::DigitalOut _producerLed;
  zpp_lib::DigitalOut _consumerLed;
  zpp_lib::Mutex _producerConsumerMutex;
  zpp_lib::Semaphore _outSemaphore{0, kBufferSize - 1};
  zpp_lib::Semaphore _inSemaphore{kBufferSize - 1, kBufferSize - 1};
  T _buffer[kBufferSize]  = {0};
  uint32_t _producerIndex = 0;
  uint32_t _consumerIndex = 0;
};

}  // namespace multi_tasking
