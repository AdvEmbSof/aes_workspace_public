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
 * @file car_system.cpp
 * @author Serge Ayer <serge.ayer@hefr.ch>
 *
 * @brief SporadicClassGenerator implementation
 *
 * @date 2025-07-01
 * @version 1.0.0
 ***************************************************************************/

#if CONFIG_APERIODIC_TASKS

#include "sporadic_task_generator.hpp"

// zephyr
#include <zephyr/logging/log.h>
#if CONFIG_USERSPACE
#include <zephyr/app_memory/app_memdomain.h>
#endif  // CONFIG_USERSPACE
#if CONFIG_SEGGER_SYSTEMVIEW
#include "SEGGER_SYSVIEW.h"
#endif  // CONFIG_SEGGER_SYSTEMVIEW

#if CONFIG_USERSPACE
extern struct k_mem_partition app_partition;
#define APP_DATA \
  K_APP_DMEM(app_partition)  // MISRA-suppress: 19.0.2 Zephyr partition-tagging macro,
                             // cannot use constexpr
#endif                       // CONFIG_USERSPACE

// stl
#include <chrono>

// local
#if CONFIG_TEST
#include "task_recorder.hpp"
#endif

LOG_MODULE_DECLARE(car_system, CONFIG_APP_LOG_LEVEL);

namespace car_system {

#if CONFIG_USERSPACE
APP_DATA char gMsgqBuffer[sizeof(std::chrono::milliseconds) * SporadicTaskGenerator::MESSAGE_QUEUE_SIZE] = {0};

SporadicTaskGenerator::SporadicTaskGenerator() : _messageQueue(gMsgqBuffer) {}
#endif  // CONFIG_USERSSPACE

void SporadicTaskGenerator::start(zpp_lib::Barrier& barrier) {
  // Wait that all threads are ready to start
  std::chrono::milliseconds startExecutionTime =
#if CONFIG_TEST
      std::chrono::duration_cast<std::chrono::milliseconds>(barrier.wait(&TaskRecorder::set_zero_time));
#else
      std::chrono::duration_cast<std::chrono::milliseconds>(barrier.wait());
#endif
  LOG_DBG("SporadicTaskGenerator Thread starting at time %lld ms", startExecutionTime.count());

  using std::literals::chrono_literals::operator""ms;
  using std::literals::chrono_literals::operator""s;
  static const std::chrono::milliseconds SporadicComputingTimes[] = {50ms, 50ms, 50ms, 50ms};
  static const std::chrono::milliseconds SporadicArrivalTimes[]   = {60ms, 300ms, 630ms, 900ms};
  static const uint32_t NbrOfSporadicRequestsPerMajorCycle        = sizeof(SporadicArrivalTimes) / sizeof(SporadicArrivalTimes[0]);

  uint32_t cycleIndex                      = 0;
  static constexpr auto majorCycleDuration = 1000ms;
  while (!_stopFlag) {
    uint32_t sporadicIndexInMajorCycle = 0;

    // generate aperiodic requests for this major cycle
    while (sporadicIndexInMajorCycle < NbrOfSporadicRequestsPerMajorCycle) {
      // wait for the next request to be generated
      auto nextTime = SporadicArrivalTimes[sporadicIndexInMajorCycle] + startExecutionTime + (cycleIndex * majorCycleDuration);
      LOG_DBG("SporadicTaskGenerator thread sleeping until %lld ms", nextTime.count());

      zpp_lib::ThisThread::sleep_until(nextTime);

      static constexpr auto timeOut     = 1s;
      zpp_lib::ZephyrBoolResult boolRes = _messageQueue.try_put_for(timeOut, SporadicComputingTimes[sporadicIndexInMajorCycle]);
      __ASSERT(!boolRes.has_error(), "Got an error from try_put_for: %d", static_cast<int>(boolRes.error()));
      if (!boolRes) {
        LOG_ERR("Could not put event to messageQueue");
      }

      sporadicIndexInMajorCycle++;
    }

    // move to next major cycle
    cycleIndex++;
  }
}

void SporadicTaskGenerator::stop() { _stopFlag = true; }

zpp_lib::ZephyrBoolResult SporadicTaskGenerator::get_sporadic_task(std::chrono::milliseconds& taskComputationTime,
                                                                   const std::chrono::milliseconds& timeOut) {
  zpp_lib::ZephyrBoolResult boolRes = _messageQueue.try_get_for(timeOut, taskComputationTime);
  __ASSERT(!boolRes.has_error(), "Got an error from try_put_for: %d", static_cast<int>(boolRes.error()));
  return boolRes;
}

zpp_lib::ZephyrBoolResult SporadicTaskGenerator::resubmit_sporadic_task(const std::chrono::milliseconds& taskComputationTime) {
  using std::literals::chrono_literals::operator""s;
  static constexpr auto timeOut     = 1s;
  zpp_lib::ZephyrBoolResult boolRes = _messageQueue.try_put_for(timeOut, taskComputationTime);
  __ASSERT(!boolRes.has_error(), "Got an error from try_put_for: %d", static_cast<int>(boolRes.error()));
  if (!boolRes) {
    LOG_ERR("Could not put event to messageQueue");
  }
  return boolRes;
}

#if CONFIG_USERSPACE
void SporadicTaskGenerator::grant_access(k_tid_t tid) { _messageQueue.grant_access(tid); }

#endif  // CONFIG_USERSPACE

}  // namespace car_system

#endif  // CONFIG_APERIODIC_TASKS
