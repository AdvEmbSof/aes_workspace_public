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
 * @file concurrent_access.cpp
 * @author Serge Ayer <serge.ayer@hefr.ch>
 *
 * @brief Implementation of the ConcurrentAccess class
 *
 * @date 2025-07-01
 * @version 1.0.0
 ***************************************************************************/
#include "concurrent_access.hpp"

// std
#include <atomic>

// zpp_lib
#include "zpp_include/this_thread.hpp"
#include "zpp_include/types.hpp"
#include "zpp_include/zpp_log.hpp"

ZPP_LOG_MODULE_DECLARE(multi_tasking, CONFIG_APP_LOG_LEVEL);

namespace multi_tasking {

ConcurrentAccess::ConcurrentAccess(const char* threadName) : _thread(zpp_lib::PreemptableThreadPriority::PriorityNormal, threadName) {}

zpp_lib::ZephyrResult ConcurrentAccess::start() {
  auto res = _thread.start([this]() { this->perform_concurrent_accesses(true); });
  if (!res) {
    ZPP_LOG_ERR("Failed to start thread: %d", static_cast<int>(res.error()));
    return res;
  }
  printk("Thread started successfully\n");
  return res;
}

void ConcurrentAccess::perform_concurrent_accesses(bool producer) {
  static constexpr uint32_t kLogInterval = 1000;
  while (true) {
    // if this would execute in an ISR, we need to make sure that the atomic operation is lock free,
    // otherwise access to a hidden mutex could be required.
    static_assert(std::atomic<uint32_t>::is_always_lock_free);

    // increment a simple counter with a fetch_add
    // memory_order_relaxed is enough because the counter is just a counter
    // and nothing else depends on its value for ordering.
    _counter.fetch_add(1, std::memory_order_relaxed);
    if (_counter.load(std::memory_order_relaxed) % kLogInterval == 0) {
      ZPP_LOG_INF(
          "Counter value is %" PRIu32 " (thread: %p)", _counter.load(std::memory_order_relaxed), static_cast<void*>(k_current_get()));
    }

    // producer/consumer scenario with release/acquire semantics
    // store(release) guarantees that all writes before the store are visible
    // to a thread that does a load(acquire) and sees this value.
    if (producer) {
      // Wait until the consumer has consumed the previous data
      // This is a busy wait, but in a real application you would use a semaphore or condition variable to avoid wasting CPU cycles.
      while (_data_ready.load(std::memory_order_acquire)) {
      }
      _shared_data.att1++;  //_counter.load(std::memory_order_relaxed);//_counter.load(std::memory_order_relaxed);
      _shared_data.att2++;
      _data_ready.store(true, std::memory_order_release);  // publish
    } else {
      // Wait until the producer has produced the next data
      // This is a busy wait, but in a real application you would use a semaphore or condition variable to avoid wasting CPU cycles.
      while (!_data_ready.load(std::memory_order_acquire)) {
      }
      uint32_t att1 = _shared_data.att1;
      uint32_t att2 = _shared_data.att2;                    // counter.load(std::memory_order_relaxed);
      _data_ready.store(false, std::memory_order_release);  // hand the buffer back
      ZPP_ASSERT(att1 == att2, "Data inconsistency: att1=%u, att2=%u", att1, att2);
      if (att1 % kLogInterval == 0) {
        ZPP_LOG_INF("Data consistency: att1=%u, att2=%u (thread: %p)", att1, att2, static_cast<void*>(k_current_get()));
      }
    }

    // read and modify pattern with release/acquire semantics
    // First, each thread writes its own field (no race: different variables)

    // Each thread registers the finished value before arriving at the barrier, and then waits for the other thread to arrive.
    uint32_t finished = _finished.load(std::memory_order_acquire);

    if (producer) {
      _own_data.att1++;
    } else {
      _own_data.att2++;
    }

    // acq_rel: release my write, acquire the other thread's write
    // We introduce a barrier-like wait with busy waiting to make sure
    if (_arrived.fetch_add(1, std::memory_order_acq_rel) == 1) {
      // I'm the second to finish: both writes are visible here
      // My acquire synchronizes with the first thread's release, so I see the first thread's writes.
      uint32_t att1 = _own_data.att1;
      uint32_t att2 = _own_data.att2;
      ZPP_ASSERT(att1 == att2, "att1=%u, att2=%u", att1, att2);
      if (att1 % kLogInterval == 0) {
        ZPP_LOG_INF("Data consistency: att1=%u, att2=%u (thread: %p, _finished %d)",
                    att1,
                    att2,
                    static_cast<void*>(k_current_get()),
                    _finished.load(std::memory_order_relaxed));
      }

      _arrived.store(0, std::memory_order_relaxed);              // reset for next round
      _finished.store(finished + 1, std::memory_order_release);  // release the waiting thread
    } else {
      // I'm the first to finish: wait for the other thread to finish
      // This is a busy wait
      while (_finished.load(std::memory_order_acquire) == finished) {
      }
    }

    // QUESTION: compare the use of _counter vs _finished
    // Ask: "Does anyone make a decision about other memory based on the value I read from this atomic?"
    // No → relaxed (plain counters, statistics, ID generators, event counts)
    // Yes → you need acquire/release (flags, reference counts, "done" markers, lock-like constructs)

    // One thread could start the next iteration before the other thread has finished
    // However both threads will synchronize  iteration, but this is not a problem because each thread only modifies its own data and the
    // atomic operations ensure that the other thread sees the updates.
  }
}

}  // namespace multi_tasking
