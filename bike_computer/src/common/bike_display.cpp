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
 * @file bike_display.cpp
 * @author Serge Ayer <serge.ayer@hefr.ch>
 *
 * @brief Bike Display header file
 *
 * @date 2025-07-01
 * @version 1.0.0
 ***************************************************************************/

#include "bike_display.hpp"

// zephyr

// std
#include <cstdio>
#include <format>
#include <string>

// zpp_lib
#include "zpp_include/zpp_log.hpp"
#include "zpp_include/zpp_assert.hpp"

// icons and fonts
#include "resources/distance_icon_50.hpp"
#include "resources/fonts.hpp"
#include "resources/gear_icon_50.hpp"
#include "resources/speedometer_icon_50.hpp"
#include "resources/thermometer_icon_50.hpp"

ZPP_LOG_MODULE_DECLARE(bike_computer, CONFIG_APP_LOG_LEVEL);

namespace bike_computer {

#if CONFIG_DISPLAY

// constants
static constexpr uint32_t kTitleHeight = 60;

// type definitions for icons
struct ImageInfo {
  const uint32_t* p_image_data;
  uint8_t image_width;
  uint8_t image_height;
};

static constexpr uint8_t kNbrOfIcons              = 4;
// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays) -- static data known at compile time
static constexpr ImageInfo kImageInfo[kNbrOfIcons] = {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay) -- static data known at compile time
  {.p_image_data = speedometer_icon, .image_width = kSpeedometerIconWidth, .image_height = kSpeedometerIconHeight},
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay) -- static data known at compile time
  {.p_image_data = gear_icon, .image_width = kGearIconWidth, .image_height = kGearIconHeight},
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay) -- static data known at compile time
  {.p_image_data = thermometer_icon, .image_width = kThermometerIconWidth, .image_height = kThermometerIconHeight},
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay) -- static data known at compile time
  {.p_image_data = distance_icon, .image_width = kDistanceIconWidth, .image_height = kDistanceIconHeight}
};

zpp_lib::ZephyrResult BikeDisplay::initialize() {
  // initialize the display
  auto res = _display.initialize();
  if (!res) {
    ZPP_LOG_ERR("Failed to initialized display: %d", (int)res.error());
    return res;
  }

  ZPP_LOG_DBG("Display initialized");
  
  // compute positions
  compute_positions();

  // display title
  display_title();

  // display icons (with lines)
  display_icons();

  // setup font for printing bike info
  _display.set_background_color(zpp_lib::Display::Color::White);

  return res;
}

void BikeDisplay::compute_positions() {
  // horizontal and vertical lines below title bar
  _vert_line_xpos  = _display.get_width() / 2;
  _info_box_height = (_display.get_height() - kTitleHeight);
  _hor_line_ypos   = kTitleHeight + (_info_box_height / 2);
  // speed (top left)
  _speedometer_icon_xpos            = kIconXMargin;
  _speedometer_icon_ypos            = _hor_line_ypos - (_info_box_height / 4) - (kImageInfo[kSpeedometerIndex].image_height / 2);
  uint32_t speedo_text_box_width = _vert_line_xpos - _speedometer_icon_xpos;
  _speedometer_text_mid_xpos        = _speedometer_icon_xpos + (speedo_text_box_width / 2) + kTextXMargin;
  _speedometer_text_ypos            = _hor_line_ypos - (_info_box_height / 4);
  // distance (bottom left)
  _distance_icon_xpos             = kIconXMargin;
  _distance_icon_ypos             = _hor_line_ypos + (_info_box_height / 4) - (kImageInfo[kDistanceIndex].image_height / 2);
  uint32_t dist_text_box_width = _vert_line_xpos - _distance_icon_xpos;
  _distance_text_mid_xpos         = _distance_icon_xpos + (dist_text_box_width / 2) + kTextXMargin;
  _distance_text_ypos             = _hor_line_ypos + (_info_box_height / 4);
  // temperature (top right)
  _temperature_icon_xpos          = _vert_line_xpos + kIconXMargin;
  _temperature_icon_ypos          = _hor_line_ypos - (_info_box_height / 4) - (kImageInfo[kTemperatureIndex].image_height / 2);
  uint32_t temp_text_box_width = _display.get_width() - _temperature_icon_xpos;
  _temperature_text_mid_xpos      = _temperature_icon_xpos + (temp_text_box_width / 2) + kTextXMargin;
  _temperature_text_ypos          = _hor_line_ypos - (_info_box_height / 4);
  // gear (bottom right)
  _gear_icon_xpos                 = _vert_line_xpos + kIconXMargin;
  _gear_icon_ypos                 = _hor_line_ypos + (_info_box_height / 4) - (kImageInfo[kGearIndex].image_height / 2);
  uint32_t gear_text_box_width = _display.get_width() - _gear_icon_xpos;
  _gear_text_mid_xpos             = _gear_icon_xpos + (gear_text_box_width / 2) + kTextXMargin;
  _gear_text_ypos                 = _hor_line_ypos + (_info_box_height / 4);
}

void BikeDisplay::display_title() {
  _display.fill_display(zpp_lib::Display::Color::White);
  _display.fill_rectangle(zpp_lib::Display::Color::Blue, 0, 0, _display.get_width(), kTitleHeight);
  _display.set_background_color(zpp_lib::Display::Color::Blue);
  _display.set_font(get_font18());
  _display.draw_string_at_line(zpp_lib::Display::Color::White, 1, "Bike Computer", zpp_lib::Display::AlignMode::Center);
}

void BikeDisplay::display_icons() {
  // draw the vertical and horizontal lines
  _display.draw_vertical_line(_color, _vert_line_xpos, kTitleHeight, _display.get_height() - kTitleHeight, kLineThickness);
  _display.draw_horizontal_line(_color, 0, _hor_line_ypos, _display.get_width(), kLineThickness);

  // draw the speedometer icon
  _display.draw_icon(_speedometer_icon_xpos,
                     _speedometer_icon_ypos,
                     kImageInfo[kSpeedometerIndex].p_image_data,
                     kImageInfo[kSpeedometerIndex].image_width,
                     kImageInfo[kSpeedometerIndex].image_height);

  // draw the distance icon
  _display.draw_icon(_distance_icon_xpos,
                     _distance_icon_ypos,
                     kImageInfo[kDistanceIndex].p_image_data,
                     kImageInfo[kDistanceIndex].image_width,
                     kImageInfo[kDistanceIndex].image_height);

  // draw the temperature icon
  _display.draw_icon(_temperature_icon_xpos,
                     _temperature_icon_ypos,
                     kImageInfo[kTemperatureIndex].p_image_data,
                     kImageInfo[kTemperatureIndex].image_width,
                     kImageInfo[kTemperatureIndex].image_height);

  // draw the gear icon
  _display.draw_icon(_gear_icon_xpos,
                     _gear_icon_ypos,
                     kImageInfo[kGearIndex].p_image_data,
                     kImageInfo[kGearIndex].image_width,
                     kImageInfo[kGearIndex].image_height);
}

void BikeDisplay::display_gear(uint8_t gear) {
  _display.set_font(get_font18());
  // std::format produces a link error when compiled for qemu_x86, so we use snprintf instead
  // std::string str    = std::format("{}", static_cast<uint32_t>(gear));
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  char buf[32] = {};
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  snprintf(buf, sizeof(buf), "%1d", gear);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  std::string str(buf);
  uint32_t str_width = _display.get_string_width(str);
  uint32_t text_xpos = _gear_text_mid_xpos - (str_width / 2);
  uint32_t text_ypos = _gear_text_ypos - (_display.get_font()->height / 2);
  _display.draw_string_at(_color, text_xpos, text_ypos, str);
}

inline std::string print_msg(double value, const char* unit) {
  // std::format produces a link error when compiled for qemu_x86, so we use snprintf instead
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
  char buf[32] = {};
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  snprintf(buf, sizeof(buf), "%.2f %s", value, unit);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  return {buf};
}

void BikeDisplay::display_speed(float speed) {
  _display.set_font(get_font16());
  // std::format produces a link error when compiled for qemu_x86, so we use snprintf instead
  std::string str = print_msg(speed, "km/h");
  uint32_t str_width = _display.get_string_width(str);
  uint32_t text_xpos = _speedometer_text_mid_xpos - (str_width / 2);
  uint32_t text_ypos = _speedometer_text_ypos - (_display.get_font()->height / 2);
  _display.draw_string_at(_color, text_xpos, text_ypos, str);
}

void BikeDisplay::display_distance(float distance) {
  _display.set_font(get_font16());
  // std::format produces a link error when compiled for qemu_x86, so we use snprintf instead
  std::string str = print_msg(distance, "km");
  uint32_t str_width = _display.get_string_width(str);
  uint32_t text_xpos = _distance_text_mid_xpos - (str_width / 2);
  uint32_t text_ypos = _distance_text_ypos - (_display.get_font()->height / 2);
  _display.draw_string_at(_color, text_xpos, text_ypos, str);
}

void BikeDisplay::display_temperature(float temperature) {
  _display.set_font(get_font16());
  // std::format produces a link error when compiled for qemu_x86, so we use snprintf instead
  std::string str = print_msg(temperature, "°C");
  uint32_t str_width = _display.get_string_width(str);
  uint32_t text_xpos = _temperature_text_mid_xpos - (str_width / 2);
  uint32_t text_ypos = _temperature_text_ypos - (_display.get_font()->height / 2);
  _display.draw_string_at(_color, text_xpos, text_ypos, str);
}

#endif  // CONFIG_DISPLAY

}  // namespace bike_computer
