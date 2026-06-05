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
 * @file bike_display.hpp
 * @author Serge Ayer <serge.ayer@hefr.ch>
 *
 * @brief Bike Display header file
 *
 * @date 2025-07-01
 * @version 1.0.0
 ***************************************************************************/

#pragma once

// zpp_lib
#include "zpp_include/display.hpp"
#include "zpp_include/zephyr_result.hpp"

namespace bike_computer {

#if CONFIG_DISPLAY == 1

class BikeDisplay {
public:
  // constructor
  BikeDisplay() = default;

  // to be called prior to any other method
  zpp_lib::ZephyrResult initialize();

  void display_gear(uint8_t gear);
  void display_speed(float speed);
  void display_distance(float distance);
  void display_temperature(float temperature);
  void reset();

private:
  // private methods
  void display_title();
  void display_info();
  void display_icons();
  void draw_lines();
  void compute_positions();

  zpp_lib::Display _display;
  zpp_lib::Display::Color _color = zpp_lib::Display::Color::Blue;

  static constexpr uint32_t kLineThickness   = 2;
  static constexpr uint32_t kIconXMargin     = 15;
  static constexpr uint32_t kTextXMargin     = 30;
  static constexpr uint8_t kSpeedometerIndex = 0;
  static constexpr uint8_t kGearIndex        = 1;
  static constexpr uint8_t kTemperatureIndex = 2;
  static constexpr uint8_t kDistanceIndex    = 3;
  uint32_t _hor_line_ypos                    = 0;
  uint32_t _vert_line_xpos                   = 0;
  uint32_t _info_box_height                  = 0;
  uint32_t _speedometer_icon_xpos            = 0;
  uint32_t _speedometer_icon_ypos            = 0;
  uint32_t _speedometer_text_mid_xpos        = 0;
  uint32_t _speedometer_text_ypos            = 0;
  uint32_t _gear_icon_xpos                   = 0;
  uint32_t _gear_icon_ypos                   = 0;
  uint32_t _gear_text_mid_xpos               = 0;
  uint32_t _gear_text_ypos                   = 0;
  uint32_t _temperature_icon_xpos            = 0;
  uint32_t _temperature_icon_ypos            = 0;
  uint32_t _temperature_text_mid_xpos        = 0;
  uint32_t _temperature_text_ypos            = 0;
  uint32_t _distance_icon_xpos               = 0;
  uint32_t _distance_icon_ypos               = 0;
  uint32_t _distance_text_mid_xpos           = 0;
  uint32_t _distance_text_ypos               = 0;
};

#else
// default dummy BikeDisplay
class BikeDisplay {
public:
  // constructor
  BikeDisplay() = default;
  zpp_lib::ZephyrResult initialize() {
    return zpp_lib::ZephyrResult();
  }
  void display_gear(uint8_t gear) {}
  void display_speed(float speed) {}
  void display_distance(float distance) {}
  void display_temperature(float temperature) {}
};

#endif // CONFIG_DISPLAY == 1

} // namespace bike_computer
