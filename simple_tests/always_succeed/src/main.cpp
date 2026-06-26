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
 * @file test.cpp
 * @author Serge Ayer <serge.ayer@hefr.ch>
 *
 * @brief Test program for demonstrating the basic test features
 *
 * @date 2025-07-01
 * @version 1.0.0
 ***************************************************************************/

// zpp_lib
#include "zpp_include/zpp_assert.hpp"
#include "zpp_include/zpp_test.hpp"

ZPP_ZTEST(always_succeed, test_equality) {
  // this is the always succeed test
  zpp_zassert_true(4 == 2 * 2);
}

ZPP_ZTEST_SUITE(always_succeed, nullptr, nullptr, nullptr, nullptr, nullptr);
