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
#include <zephyr/logging/log.h>

// std
#include <cstdio>
#include <format>
#include <string>

// zpp_lib

// icons and fonts
#include "resources/distance_icon_50.hpp"
#include "resources/fonts.hpp"
#include "resources/gear_icon_50.hpp"
#include "resources/speedometer_icon_50.hpp"
#include "resources/thermometer_icon_50.hpp"

LOG_MODULE_DECLARE(bike_computer, CONFIG_APP_LOG_LEVEL);

namespace bike_computer {

#if CONFIG_DISPLAY == 1

// constants
static constexpr uint32_t kTitleHeight = 60;

// type definitions for logos
struct Logos {
  struct ImageInfo {
    const uint32_t* pImageData;
    const uint8_t imageWidth;
    const uint8_t imageHeight;
  };

  static constexpr uint8_t kNbrOfImages               = 4;
  static constexpr ImageInfo _imageInfo[kNbrOfImages] = {
      {.pImageData = speedometer_icon, .imageWidth = kSpeedometerIconWidth, .imageHeight = kSpeedometerIconHeight},
      {.pImageData = gear_icon, .imageWidth = kGearIconWidth, .imageHeight = kGearIconHeight},
      {.pImageData = thermometer_icon, .imageWidth = kThermometerIconWidth, .imageHeight = kThermometerIconHeight},
      {.pImageData = distance_icon, .imageWidth = kDistanceIconWidth, .imageHeight = kDistanceIconHeight}};
};
static const Logos gLogos;

zpp_lib::ZephyrResult BikeDisplay::initialize() {
  // initialize the display
  auto res = _display.initialize();
  if (!res) {
    LOG_ERR("Failed to initialized display: %d", (int)res.error());
    return res;
  } else {
    LOG_DBG("Display initialized");
  }

  // compute positions
  computePositions();

  // display title
  displayTitle();

  // display icons (with lines)
  displayIcons();

  // setup font for printing bike info
  _display.set_background_color(zpp_lib::Display::Color::White);

  return res;
}

void BikeDisplay::computePositions() {
  // horizontal and vertical lines below title bar
  _vert_line_xpos  = _display.get_width() / 2;
  _info_box_height = (_display.get_height() - kTitleHeight);
  _hor_line_ypos   = kTitleHeight + _info_box_height / 2;
  // speed (top left)
  _speedometer_icon_xpos            = kIconXMargin;
  _speedometer_icon_ypos            = _hor_line_ypos - _info_box_height / 4 - gLogos._imageInfo[kSpeedometerIndex].imageHeight / 2;
  const uint32_t speedoTextBoxWidth = _vert_line_xpos - _speedometer_icon_xpos;
  _speedometer_text_mid_xpos        = _speedometer_icon_xpos + speedoTextBoxWidth / 2 + kTextXMargin;
  _speedometer_text_ypos            = _hor_line_ypos - _info_box_height / 4;
  // distance (bottom left)
  _distance_icon_xpos             = kIconXMargin;
  _distance_icon_ypos             = _hor_line_ypos + _info_box_height / 4 - gLogos._imageInfo[kDistanceIndex].imageHeight / 2;
  const uint32_t distTextBoxWidth = _vert_line_xpos - _distance_icon_xpos;
  _distance_text_mid_xpos         = _distance_icon_xpos + distTextBoxWidth / 2 + kTextXMargin;
  _distance_text_ypos             = _hor_line_ypos + _info_box_height / 4;
  // temperature (top right)
  _temperature_icon_xpos          = _vert_line_xpos + kIconXMargin;
  _temperature_icon_ypos          = _hor_line_ypos - _info_box_height / 4 - gLogos._imageInfo[kTemperatureIndex].imageHeight / 2;
  const uint32_t tempTextBoxWidth = _display.get_width() - _temperature_icon_xpos;
  _temperature_text_mid_xpos      = _temperature_icon_xpos + tempTextBoxWidth / 2 + kTextXMargin;
  _temperature_text_ypos          = _hor_line_ypos - _info_box_height / 4;
  // gear (bottom right)
  _gear_icon_xpos                 = _vert_line_xpos + kIconXMargin;
  _gear_icon_ypos                 = _hor_line_ypos + _info_box_height / 4 - gLogos._imageInfo[kGearIndex].imageHeight / 2;
  const uint32_t gearTextBoxWidth = _display.get_width() - _gear_icon_xpos;
  _gear_text_mid_xpos             = _gear_icon_xpos + gearTextBoxWidth / 2 + kTextXMargin;
  _gear_text_ypos                 = _hor_line_ypos + _info_box_height / 4;
}

void BikeDisplay::displayTitle() {
  _display.fill_display(zpp_lib::Display::Color::White);
  _display.fill_rectangle(zpp_lib::Display::Color::Blue, 0, 0, _display.get_width(), kTitleHeight);
  _display.set_background_color(zpp_lib::Display::Color::Blue);
  _display.set_font(get_font18());
  _display.draw_string_at_line(zpp_lib::Display::Color::White, 1, "Bike Computer", zpp_lib::Display::AlignMode::Center);
}

void BikeDisplay::displayIcons() {
  // draw the vertical and horizontal lines
  _display.draw_vertical_line(_color, _vert_line_xpos, kTitleHeight, _display.get_height() - kTitleHeight, kLineThickness);
  _display.draw_horizontal_line(_color, 0, _hor_line_ypos, _display.get_width(), kLineThickness);

  // draw the speedometer icon
  _display.draw_icon(_speedometer_icon_xpos,
                     _speedometer_icon_ypos,
                     const_cast<uint32_t*>(gLogos._imageInfo[kSpeedometerIndex].pImageData),
                     gLogos._imageInfo[kSpeedometerIndex].imageWidth,
                     gLogos._imageInfo[kSpeedometerIndex].imageHeight);

  // draw the distance icon
  _display.draw_icon(_distance_icon_xpos,
                     _distance_icon_ypos,
                     const_cast<uint32_t*>(gLogos._imageInfo[kDistanceIndex].pImageData),
                     gLogos._imageInfo[kDistanceIndex].imageWidth,
                     gLogos._imageInfo[kDistanceIndex].imageHeight);

  // draw the temperature icon
  _display.draw_icon(_temperature_icon_xpos,
                     _temperature_icon_ypos,
                     const_cast<uint32_t*>(gLogos._imageInfo[kTemperatureIndex].pImageData),
                     gLogos._imageInfo[kTemperatureIndex].imageWidth,
                     gLogos._imageInfo[kTemperatureIndex].imageHeight);

  // draw the gear icon
  _display.draw_icon(_gear_icon_xpos,
                     _gear_icon_ypos,
                     const_cast<uint32_t*>(gLogos._imageInfo[kGearIndex].pImageData),
                     gLogos._imageInfo[kGearIndex].imageWidth,
                     gLogos._imageInfo[kGearIndex].imageHeight);
}

void BikeDisplay::displayGear(uint8_t gear) {
  _display.set_font(get_font18());
  // std::format produces a link error when compiled for qemu_x86, so we use snprintf instead
  // std::string str    = std::format("{}", static_cast<uint32_t>(gear));
  char buf[32] = {};
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  snprintf(buf, sizeof(buf), "%1d", gear);
  std::string str(buf);
  const uint32_t str_width = _display.get_string_width(str);
  const uint32_t text_xpos = _gear_text_mid_xpos - str_width / 2;
  const uint32_t text_ypos = _gear_text_ypos - _display.get_font()->height / 2;
  _display.draw_string_at(_color, text_xpos, text_ypos, str);
}

void BikeDisplay::displaySpeed(float speed) {
  _display.set_font(get_font16());
  // std::format produces a link error when compiled for qemu_x86, so we use snprintf instead
  // std::string str    = std::format("{:.1f} km/h", static_cast<double>(speed));
  char buf[32] = {};
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  snprintf(buf, sizeof(buf), "%.1f km/h", static_cast<double>(speed));
  std::string str(buf);
  const uint32_t str_width = _display.get_string_width(str);
  const uint32_t text_xpos = _speedometer_text_mid_xpos - str_width / 2;
  const uint32_t text_ypos = _speedometer_text_ypos - _display.get_font()->height / 2;
  _display.draw_string_at(_color, text_xpos, text_ypos, str);
}

void BikeDisplay::displayDistance(float distance) {
  _display.set_font(get_font16());
  // std::format produces a link error when compiled for qemu_x86, so we use snprintf instead
  // std::string str    = "";//std::format("{:.2f} km", static_cast<double>(distance));
  char buf[32] = {};
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  snprintf(buf, sizeof(buf), "%.2f km", static_cast<double>(distance));
  std::string str(buf);
  const uint32_t str_width = _display.get_string_width(str);
  const uint32_t text_xpos = _distance_text_mid_xpos - str_width / 2;
  const uint32_t text_ypos = _distance_text_ypos - _display.get_font()->height / 2;
  _display.draw_string_at(_color, text_xpos, text_ypos, str);
}

void BikeDisplay::displayTemperature(float temperature) {
  _display.set_font(get_font16());
  // std::format produces a link error when compiled for qemu_x86, so we use snprintf instead
  // std::string str    = std::format("{:.1f}°C", static_cast<double>(temperature));
  char buf[32] = {};
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  snprintf(buf, sizeof(buf), "%.1f\260C", static_cast<double>(temperature));
  std::string str(buf);
  const uint32_t str_width = _display.get_string_width(str);
  const uint32_t text_xpos = _temperature_text_mid_xpos - (str_width / 2);
  const uint32_t text_ypos = _temperature_text_ypos - (_display.get_font()->height / 2);
  _display.draw_string_at(_color, text_xpos, text_ypos, str);
}

#endif  // CONFIG_DISPLAY == 1

}  // namespace bike_computer
