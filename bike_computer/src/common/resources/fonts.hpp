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
 * @file fonts.hpp
 * @author Serge Ayer <serge.ayer@hefr.ch>
 *
 * @brief Fonts interface
 *
 * @date 2025-07-01
 * @version 1.0.0
 ***************************************************************************/

#pragma once

#if CONFIG_DISPLAY == 1

// zpp_lib
#include "zpp_include/display.hpp"

namespace bike_computer {

extern zpp_lib::Display::Font* getFont12();
extern zpp_lib::Display::Font* getFont14();
extern zpp_lib::Display::Font* getFont16();
extern zpp_lib::Display::Font* getFont18();
extern zpp_lib::Display::Font* getFont26b();
extern zpp_lib::Display::Font* getFont36b();

}  // namespace bike_computer

#endif  // CONFIG_DISPLAY == 1
