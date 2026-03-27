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
 * @file periodic_task_info.hpp
 * @author Serge Ayer <serge.ayer@hefr.ch>
 *
 * @brief Periodic Task Info definition
 *
 * @date 2025-07-01
 * @version 1.0.0
 ***************************************************************************/

#pragma once

// std
#include <chrono>

// zpp_lib
#include "zpp_include/mutex.hpp"
#include "zpp_include/types.hpp"

namespace car_system {

#if CONFIG_PHASE_C
struct SubtaskComputationInfo {
  std::chrono::milliseconds _computationTime;
  zpp_lib::Mutex* _pMutex;
};
static constexpr uint8_t NbrOfSubTasks = 3;
#endif  // CONFIG_PHASE_C

struct PeriodicTaskInfo {
#if CONFIG_PHASE_C
  SubtaskComputationInfo _subTasks[NbrOfSubTasks];
#else   // CONFIG_PHASE_C
  std::chrono::milliseconds _computationTime;
#endif  // CONFIG_PHASE_C
  std::chrono::milliseconds _period;
  zpp_lib::PreemptableThreadPriority _priority;
  const char* _szTaskName;
};

}  // namespace car_system
