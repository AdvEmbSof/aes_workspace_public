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
 * @file buffer_queue.hpp
 * @author Serge Ayer <serge.ayer@hefr.ch>
 *
 * @brief Declaration/Implementation of the Buffer class (using MessageQueue)
 *
 * @date 2025-07-01
 * @version 1.0.0
 ***************************************************************************/

#pragma once
// zephyr
#include <zephyr/random/random.h>

// zpp_lib
#include "zpp_include/digital_out.hpp"
#include "zpp_include/message_queue.hpp"
#include "zpp_include/this_thread.hpp"

namespace multi_tasking {

using namespace std::literals;

static constexpr uint8_t kLedOff = 0;
static constexpr uint8_t kLedOn  = 1;

class Buffer {
 public:
  Buffer()
      : _producerLed(zpp_lib::DigitalOut::PinName::LED0, kLedOff),
        _consumerLed(zpp_lib::DigitalOut::PinName::LED1, kLedOff) {}

  uint32_t append(uint32_t data) {
    _producerLed = kLedOn;

    zpp_lib::ThisThread::busyWait(computeRandomWaitTime(kApppendWaitTime));

    std::chrono::milliseconds timeout = std::chrono::milliseconds::max();
    auto res                          = _messageQueue.try_put_for(timeout, data);
    if (res.has_error()) {
      __ASSERT(false, "Error getting message from queue: %d", (int)res.error());
      return 0;
    }
    if (!res) {
      __ASSERT(false, "Timeout when getting message from queue");
      return 0;
    }

    _producerLed = kLedOff;

    return _producerIndex++;
  }

  uint32_t extract(uint32_t& data) {
    _consumerLed = kLedOn;

    zpp_lib::ThisThread::busyWait(computeRandomWaitTime(kExtractWaitTime));
    std::chrono::milliseconds timeout = std::chrono::milliseconds::max();
    auto res                          = _messageQueue.try_get_for(timeout, data);
    if (res.has_error()) {
      __ASSERT(false, "Error getting message from queue: %d", (int)res.error());
      return 0;
    }
    if (!res) {
      __ASSERT(false, "Timeout when getting message from queue");
      return 0;
    }

    _consumerLed = kLedOff;
    return _consumerIndex++;
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
  zpp_lib::MessageQueue<uint32_t, kBufferSize> _messageQueue;
  uint32_t _producerIndex = 0;
  uint32_t _consumerIndex = 0;
};

}  // namespace multi_tasking
