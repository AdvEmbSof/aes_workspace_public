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
#include "zpp_include/non_copyable.hpp"
#include "zpp_include/semaphore.hpp"
#include "zpp_include/this_thread.hpp"
#include "zpp_include/zpp_assert.hpp"

namespace multi_tasking {

using std::literals::chrono_literals::operator""ms;

static constexpr bool kLedOff = false;
static constexpr bool kLedOn  = true;

template <typename T> class Buffer : public zpp_lib::NonCopyable {
public:
  Buffer() : _producer_led(zpp_lib::DigitalOut::PinName::LED0, kLedOff), _consumer_led(zpp_lib::DigitalOut::PinName::LED1, kLedOff) {}

  uint32_t append(const T& data) {
    // make sure that we can produce without overflow
    auto res = _in_semaphore.acquire();
    ZPP_ASSERT(res, "Cannot acquire inSemaphore: %d", (int)res.error());

    // lock buffer
    res = _producer_consumer_mutex.lock();
    ZPP_ASSERT(res, "Cannot lock mutex: %d", (int)res.error());

    _producer_led = kLedOn;
    // _producer_index is updated with % kBufferSize
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    _buffer[_producer_index] = data;
    uint32_t index           = _producer_index;
    _producer_index          = (_producer_index + 1) % kBufferSize;

    zpp_lib::ThisThread::busy_wait(compute_random_wait_time(kApppendWaitTime));
    _producer_led = kLedOff;

    // unlock buffer
    res = _producer_consumer_mutex.unlock();
    ZPP_ASSERT(res, "Cannot unlock mutex: %d", (int)res.error());

    // tell that one element is available for consumer
    res = _out_semaphore.release();
    ZPP_ASSERT(res, "Cannot release outSemaphore: %d", (int)res.error());

    return index;
  }

  uint32_t extract(T& data) {
    // make sure that we can consume without underflow
    auto res = _out_semaphore.acquire();
    ZPP_ASSERT(res, "Cannot acquire outSemaphore: %d", (int)res.error());

    // lock buffer
    res = _producer_consumer_mutex.lock();
    ZPP_ASSERT(res, "Cannot lock mutex: %d", (int)res.error());

    _consumer_led = kLedOn;
    // _consumer_index is updated with % kBufferSize
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    data            = _buffer[_consumer_index];
    uint32_t index  = _consumer_index;
    _consumer_index = (_consumer_index + 1) % kBufferSize;

    zpp_lib::ThisThread::busy_wait(compute_random_wait_time(kExtractWaitTime));
    _consumer_led = kLedOff;

    // unlock buffer
    res = _producer_consumer_mutex.unlock();
    ZPP_ASSERT(res, "Cannot unlock mutex: %d", (int)res.error());

    // tell that one element is available for producer
    res = _in_semaphore.release();
    ZPP_ASSERT(res, "Cannot release inSemaphore: %d", (int)res.error());

    return index;
  }

  std::chrono::milliseconds compute_random_wait_time(const std::chrono::milliseconds& wait_time) {
    return std::chrono::milliseconds((sys_rand32_get() % wait_time.count()) + wait_time.count());
  }

private:
  static constexpr std::chrono::milliseconds kApppendWaitTime = 500ms;
  static constexpr std::chrono::milliseconds kExtractWaitTime = 500ms;
  static constexpr uint8_t kBufferSize                        = 10;
  zpp_lib::DigitalOut _producer_led;
  zpp_lib::DigitalOut _consumer_led;
  zpp_lib::Mutex _producer_consumer_mutex;
  zpp_lib::Semaphore _out_semaphore{0, kBufferSize - 1};
  zpp_lib::Semaphore _in_semaphore{kBufferSize - 1, kBufferSize - 1};
  // kBufferSize is a constant, so using a c array is safe
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  T _buffer[kBufferSize]   = {0};
  uint32_t _producer_index = 0;
  uint32_t _consumer_index = 0;
};

}  // namespace multi_tasking
