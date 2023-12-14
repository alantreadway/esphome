//
// Created by Clyde Stubbs on 29/10/2023.
//
#pragma once

#ifdef USE_ESP_IDF
#include "esphome/core/component.h"
#include "esphome/components/spi/spi.h"
#include "esphome/components/display/display.h"
#include "esphome/components/display/display_buffer.h"
#include "esphome/components/display/display_color_utils.h"
#include "esp_lcd_panel_ops.h"

#include "esp_lcd_panel_rgb.h"

namespace esphome {
namespace qspi_amoled {

constexpr static const char *const TAG = "display.qspi_amoled";
static const uint8_t SW_RESET_CMD = 0x01;
static const uint8_t SLEEP_OUT = 0x11;
static const uint8_t SDIR_CMD = 0xC7;
static const uint8_t MADCTL_CMD = 0x36;
static const uint8_t INVERT_OFF = 0x20;
static const uint8_t INVERT_ON = 0x21;
static const uint8_t DISPLAY_ON = 0x29;
static const uint8_t CMD2_BKSEL = 0xFF;
static const uint8_t PIXFMT = 0x3A;
static const uint8_t BRIGHTNESS = 0x51;
static const uint8_t RASET = 0x2B;
static const uint8_t CASET = 0x2A;
static const uint8_t WDATA = 0x2C;
static const uint8_t MADCTL_MY = 0x80;   ///< Bit 7 Bottom to top
static const uint8_t MADCTL_MX = 0x40;   ///< Bit 6 Right to left
static const uint8_t MADCTL_MV = 0x20;   ///< Bit 5 Reverse Mode
static const uint8_t MADCTL_RGB = 0x00;  ///< Bit 3 Red-Green-Blue pixel order
static const uint8_t MADCTL_BGR = 0x08;  ///< Bit 3 Blue-Green-Red pixel order

// store a 16 bit value in a buffer, big endian.
static inline void put16_be(uint8_t *buf, uint16_t value) {
  buf[0] = value >> 8;
  buf[1] = value;
}

class QSPI_AMOLED : public display::DisplayBuffer,
                    public spi::SPIDevice<spi::BIT_ORDER_MSB_FIRST, spi::CLOCK_POLARITY_LOW, spi::CLOCK_PHASE_LEADING,
                                          spi::DATA_RATE_1MHZ> {
 public:
  void update() override {
    this->do_update_();
    this->display_();
  }

  void setup() override {
    esph_log_config(TAG, "Setting up QSPI_AMOLED");
    this->spi_setup();
    if (this->enable_pin_ != nullptr) {
      this->enable_pin_->setup();
      this->enable_pin_->digital_write(true);
    }
    if (this->reset_pin_ != nullptr) {
      this->reset_pin_->setup();
      this->reset_pin_->digital_write(true);
      delay(5);
      this->reset_pin_->digital_write(false);
      delay(5);
      this->reset_pin_->digital_write(true);
      delay(120);
    }
    this->write_init_sequence_();
    esph_log_config(TAG, "QSPI_AMOLED setup complete");
  }

  display::ColorOrder get_color_mode() { return this->color_mode_; }
  void set_color_mode(display::ColorOrder color_mode) { this->color_mode_ = color_mode; }
  void set_invert_colors(bool invert_colors) { this->invert_colors_ = invert_colors; }

  void set_reset_pin(GPIOPin *reset_pin) { this->reset_pin_ = reset_pin; }
  void set_enable_pin(GPIOPin *enable_pin) { this->enable_pin_ = enable_pin; }
  void set_width(uint16_t width) { this->width_ = width; }
  void set_dimensions(uint16_t width, uint16_t height) {
    this->width_ = width;
    this->height_ = height;
  }
  int get_width() override { return this->width_; }
  int get_height() override { return this->height_; }
  void set_mirror_x(bool mirror_x) {
    this->mirror_x_ = mirror_x;
    if (this->is_ready())
      this->reset_params_();
  }
  void set_mirror_y(bool mirror_y) {
    this->mirror_y_ = mirror_y;
    if (this->is_ready())
      this->reset_params_();
  }
  void set_swap_xy(bool swap_xy) {
    this->swap_xy_ = swap_xy;
    if (this->is_ready())
      this->reset_params_();
  }
  void set_brightness(uint8_t brightness) {
    this->brightness_ = brightness;
    if (this->is_ready())
      this->reset_params_();
  }
  void set_offsets(int16_t offset_x, int16_t offset_y) {
    this->offset_x_ = offset_x;
    this->offset_y_ = offset_y;
  }
  display::DisplayType get_display_type() override { return display::DisplayType::DISPLAY_TYPE_COLOR; }
  void dump_config() override;

  int get_width_internal() override { return this->width_; }
  int get_height_internal() override { return this->height_; }

 protected:
  virtual void draw_absolute_pixel_internal(int x, int y, Color color) override {
    if (this->is_failed())
      return;
    if (this->buffer_ == nullptr)
      this->init_internal_(this->width_ * this->height_ * 2);
    if (x >= this->get_width_internal() || x < 0 || y >= this->get_height_internal() || y < 0) {
      return;
    }
    uint32_t pos = (y * this->width_) + x;
    uint16_t new_color;
    bool updated = false;
    pos = pos * 2;
    new_color = display::ColorUtil::color_to_565(color, display::ColorOrder::COLOR_ORDER_RGB);
    if (this->buffer_[pos] != (uint8_t) (new_color >> 8)) {
      this->buffer_[pos] = (uint8_t) (new_color >> 8);
      updated = true;
    }
    pos = pos + 1;
    new_color = new_color & 0xFF;

    if (this->buffer_[pos] != new_color) {
      this->buffer_[pos] = new_color;
      updated = true;
    }
    if (updated) {
      // low and high watermark may speed up drawing from buffer
      if (x < this->x_low_)
        this->x_low_ = x;
      if (y < this->y_low_)
        this->y_low_ = y;
      if (x > this->x_high_)
        this->x_high_ = x;
      if (y > this->y_high_)
        this->y_high_ = y;
    }
  }

  /**
   * this relies upon the init sequence being well-formed, which is guaranteed by the Python init code.
   */

  void write_command_(uint8_t cmd, const uint8_t *bytes = nullptr, size_t len = 0) {
    this->enable();
    ESP_LOGD(TAG, "Write command %X, len %d", cmd, len);
    this->write_cmd_addr_data(8, 0x02, 24, cmd << 8, bytes, len);
    this->disable();
  }

  void reset_params_() {
    this->write_command_(this->invert_colors_ ? INVERT_ON : INVERT_OFF);
    // custom x/y transform and color order
    uint8_t mad = this->color_mode_ == display::COLOR_ORDER_BGR ? MADCTL_BGR : MADCTL_RGB;
    if (this->swap_xy_)
      mad |= MADCTL_MV;
    if (this->mirror_x_)
      mad |= MADCTL_MX;
    if (this->mirror_y_)
      mad |= MADCTL_MY;
    this->write_command_(MADCTL_CMD, &mad, 1);
    this->write_command_(BRIGHTNESS, &this->brightness_, 1);
  }

  void write_init_sequence_() {
    uint8_t data;
    this->write_command_(SLEEP_OUT);
    delay(120);   // NOLINT
    data = 0x55;  // 16 bit
    this->write_command_(PIXFMT, &data, 1);
    data = 0;
    this->write_command_(BRIGHTNESS, &data, 1);
    this->write_command_(DISPLAY_ON);
    this->reset_params_();
  }

  void set_addr_window_(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
    uint8_t buf[4];
    put16_be(buf, x1 + this->offset_x_);
    put16_be(buf + 2, x2 + this->offset_x_);
    this->write_command_(CASET, buf, sizeof buf);
    put16_be(buf, y1 + this->offset_y_);
    put16_be(buf + 2, y2 + this->offset_y_);
    this->write_command_(RASET, buf, sizeof buf);
  }

  void draw_pixels_at(int x_start, int y_start, int w, int h, const uint8_t *ptr, display::ColorOrder order,
                      display::ColorBitness bitness, bool big_endian, int x_offset, int y_offset, int x_pad) {
    if (bitness != display::COLOR_BITNESS_565 || order != this->color_mode_ ||
        big_endian != (this->bit_order_ == spi::BIT_ORDER_MSB_FIRST))
      return;  // TODO - call generic code in Display to do this one pixel at a time.
    if (w <= 0 || h <= 0)
      return;
    set_addr_window_(x_start, y_start, x_start + w - 1, y_start + h - 1);
    this->enable();
    this->write_cmd_addr_data(8, 0x32, 24, 0x2C00, nullptr, 0, 1);
    size_t pos = (x_offset + y_offset * get_width_internal()) * 2;
    esph_log_d(TAG, "draw_at %d/%d %d/%d pos=%d", x_start, y_start, w, h, pos);
    if (x_offset == 0 && x_pad == 0) {
      // can draw in one big chunk
      this->write_cmd_addr_data(0, 0, 0, 0, ptr + pos, w * h * 2, 4);
    } else {
      size_t bytes = w * 2;
      size_t stride = this->get_width_internal() * 2;
      for (size_t i = 0; i != h; i++) {
        this->write_cmd_addr_data(0, 0, 0, 0, ptr + pos, bytes, 4);
        pos += stride;
      }
    }
    this->disable();
  }

  void display_() {
    int w = this->x_high_ - this->x_low_ + 1;
    int h = this->y_high_ - this->y_low_ + 1;
    this->draw_pixels_at(this->x_low_, this->y_low_, w, h, this->buffer_, this->color_mode_, display::COLOR_BITNESS_565,
                         true, this->x_low_, this->y_low_, this->get_width_internal() - w - this->x_low_);
    // invalidate watermarks
    this->x_low_ = this->width_;
    this->y_low_ = this->height_;
    this->x_high_ = 0;
    this->y_high_ = 0;
  }

  GPIOPin *reset_pin_{nullptr};
  GPIOPin *enable_pin_{nullptr};
  uint16_t x_low_{0};
  uint16_t y_low_{0};
  uint16_t x_high_{0};
  uint16_t y_high_{0};

  bool invert_colors_{};
  display::ColorOrder color_mode_{display::COLOR_ORDER_BGR};
  size_t width_{};
  size_t height_{};
  int16_t offset_x_{0};
  int16_t offset_y_{0};
  bool swap_xy_{};
  bool mirror_x_{};
  bool mirror_y_{};
  uint8_t brightness_{0xD0};

  esp_lcd_panel_handle_t handle_{};
};

}  // namespace qspi_amoled
}  // namespace esphome
#endif
