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

// std
#include <memory>
#include <utility>

// zpp_lib
#include "zpp_include/non_copyable.hpp"
#include "zpp_include/zpp_assert.hpp"
#include "zpp_include/zpp_test.hpp"

// Structure used for testing instance counts
// We inherit from zpp_lib::NonCopyable to prevent accidental copies of the Test class
// cppcoreguidelines-special-member-functions is therefore a false positive
// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
struct Test : public zpp_lib::NonCopyable<Test> {
  Test() : _value(kMagicNumber) {
    s_instance_count++;
    
  }

  ~Test() {
    s_instance_count--;
    _value = 0;
  }

  int _value = 0;
  static constexpr uint32_t kMagicNumber = 33;
  static inline uint32_t s_instance_count = 0;
};

/**
 * Test that a shared pointer correctly manages the lifetime of the underlying raw pointer
 */
ZPP_ZTEST(ptr_test, test_single_sharedptr_lifetime) {
  // Sanity-check value of instance counter
  zpp_zassert_equal(0, Test::s_instance_count);

  // Create and destroy shared pointer in given scope
  {
    std::shared_ptr<Test> shared_ptr = std::make_shared<Test>();
    zpp_zassert_equal(1, Test::s_instance_count);
    zpp_zassert_equal(Test::kMagicNumber, shared_ptr->_value);
  }

  // Destroy shared pointer
  zpp_zassert_equal(0, Test::s_instance_count);
}

/**
 * Test that multiple instances of shared pointers correctly manage the reference count
 * to release the object at the correct point
 */
ZPP_ZTEST(ptr_test, test_instance_sharing) {
  std::shared_ptr<Test> shared_ptr1(nullptr);

  // Sanity-check value of counter
  zpp_zassert_equal(0, Test::s_instance_count);

  // Create and destroy shared pointer in given scope
  {
    std::shared_ptr<Test> shared_ptr2 = std::make_shared<Test>();
    zpp_zassert_equal(1, Test::s_instance_count);
    // share share_ptr2 with shared_ptr1
    shared_ptr1 = shared_ptr2;
    // still one instance only
    zpp_zassert_equal(1, Test::s_instance_count);
    zpp_zassert_equal(Test::kMagicNumber, shared_ptr1->_value);
    zpp_zassert_true(shared_ptr1.get() == shared_ptr2.get());
  }

  // shared_ptr1 still owns a raw pointer
  zpp_zassert_equal(1, Test::s_instance_count);

  shared_ptr1 = nullptr;

  // shared pointer has been destroyed
  zpp_zassert_equal(0, Test::s_instance_count);
}

/**
 * Test that multiple instances of unique pointers correctly manage the
 * ownership of the the object
 */
ZPP_ZTEST(ptr_test, test_unique_ptr) {
  // TO DO(student): implement this test based on the exercise description
}

/**
 * Test that when delete is not called with a raw pointer then a memory leak occurs
 */
ZPP_ZTEST(ptr_test, test_raw_pointers) {
  // TO DO(student): implement this test based on the exercise description  
}

ZPP_ZTEST_SUITE(ptr_test, nullptr, nullptr, nullptr, nullptr, nullptr);
