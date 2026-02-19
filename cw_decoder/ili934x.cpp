/*
BSD 3-Clause License

Copyright (c) 2022, Darren Horrocks
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/* Touchscreen library for XPT2046 Touch Controller Chip
 * Copyright (c) 2015, Paul Stoffregen, paul@pjrc.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice, development funding notice, and this permission
 * notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * Adapted and merged with ili934x library by Jon Dawson 2026
 */
//
#ifdef ARDUINO_ARCH_RP2040

#include "ili934x.h"
#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "stdio.h"
#include "stdlib.h"
#include <Arduino.h>
#include <cstring>

#ifndef pgm_read_byte
#define pgm_read_byte(addr) (*(const uint8_t*)(addr))
#endif
#ifndef pgm_read_word
#define pgm_read_word(addr) (*(const uint16_t*)(addr))
#endif
#ifndef pgm_read_dword
#define pgm_read_dword(addr) (*(const uint32_t*)(addr))
#endif
// #ifndef __swap_int
// #def ine __swap_int(a, b)   \
//    a = (a & b) + (a | b); \
//    b = a + (~b) + 1;      \
//    a = a + (~b) + 1;
// #endif

ILI934X::ILI934X(spi_inst_t* spi, uint8_t cs, uint8_t dc, uint16_t width, uint16_t height)
{
  m_spi = spi;
  m_cs = cs;
  m_dc = dc;
  m_init_width = width;
  m_init_height = height;
}

void ILI934X::configure_ili9488()
{

  // Positive Gamma Control
  _write(0xE0, (uint8_t*)"\x00\x03\x09\x08\x16\x0A\x3F\x78\x4C\x09\x0A\x8\x16\x1A\x0F", 15);

  // Negative Gamma Control
  _write(0XE1, (uint8_t*)"\x00\x16\x19\x03\x0F\x05\x32\x45\x46\x04\x0E\x0D\x35\x37\x0F", 15);

  // Power Control 1
  _write(0XC0, (uint8_t*)"\x17\x15", 2);

  // Power Control 2
  _write(0xC1, (uint8_t*)"\x41", 1);

  // VCOM Control
  _write(0xC5, (uint8_t*)"\x00\x12\x80", 3);

  // Power Control 2
  _write(_MADCTL, (uint8_t*)"\x48", 1);

  // Pixel Interface Format
  _write(0x3A, (uint8_t*)"\x66", 1);

  // Interface Mode Control
  _write(0xB0, (uint8_t*)"\x00", 1);

  // Frame Rate Control
  _write(0xB1, (uint8_t*)"\xA0", 1);

  // Display Inversion Control
  _write(0xB4, (uint8_t*)"\x02", 1);

  // Display Function Control
  _write(0xB6, (uint8_t*)"\x02\x02\x3B", 3);

  // Entry Mode Set
  _write(0xB7, (uint8_t*)"\xC6", 1);

  // Adjust Control 3
  _write(0xF7, (uint8_t*)"\xA9\x51\x2C\x82", 4);

  // Exit Sleep
  _write(_SLPOUT, NULL, 0);
  sleep_ms(120);

  // Display on
  _write(_DISPON, NULL, 0);
  sleep_ms(25);
}

void ILI934X::configure_st7796()
{
  sleep_ms(120);
  _write(0x01, NULL, 0);
  sleep_ms(120);
  _write(0x11, NULL, 0);
  sleep_ms(120);

  // Command Set control
  _write(0xF0, (uint8_t*)"\xC3", 1); // Enable extension command 2 partI
  _write(0xF0, (uint8_t*)"\x96", 1); // Enable extension command 2 partII
  _write(0x36, (uint8_t*)"\x48", 1);

  // Interface Pixel Format
  _write(_PIXSET, (uint8_t*)"\x55", 1); // Control interface color format set to 16

  // Column inversion
  _write(0xB4, (uint8_t*)"\x01", 1); // 1-dot inversion

  // Display Function Control
  _write(_DISCTRL, (uint8_t*)"\x80\x02\x3B", 3); // 1-dot inversion

  // Display Output Ctrl Adjust
  _write(_DTCTRLA, (uint8_t*)"\x40\x8A\x00\x00\x29\x19\xA5\x33", 8);

  // Power control2
  _write(_PWCTRL2, (uint8_t*)"\x06", 1);

  // Power control 3
  _write(_PWCTRL3, (uint8_t*)"\xA7", 1);

  // VCOM Control
  _write(_VMCTRL1, (uint8_t*)"\x18", 1);

  sleep_ms(120);

  // ST7796 Gamma Sequence
  _write(_PGAMCTRL, (uint8_t*)"\xF0\x09\x0b\x06\x04\x15\x2f\x54\x42\x3c\x17\x14\x18\x1b", 14);
  _write(_NGAMCTRL, (uint8_t*)"\xE0\x09\x0B\x06\x04\x03\x2B\x43\x42\x3B\x16\x14\x17\x1B", 14);

  sleep_ms(120);

  // Command Set control
  _write(0xF0, (uint8_t*)"\x3C", 1);
  _write(0xF0, (uint8_t*)"\x69", 1);

  sleep_ms(120);

  _write(0x29, NULL, 0);
}

void ILI934X::configure_ili934x()
{
  _write(_RDDSDR, (uint8_t*)"\x03\x80\x02", 3);
  _write(_PWCRTLB, (uint8_t*)"\x00\xc1\x30", 3);
  _write(_PWRONCTRL, (uint8_t*)"\x64\x03\x12\x81", 4);
  _write(_DTCTRLA, (uint8_t*)"\x85\x00\x78", 3);
  _write(_PWCTRLA, (uint8_t*)"\x39\x2c\x00\x34\x02", 5);
  _write(_PRCTRL, (uint8_t*)"\x20", 1);
  _write(_DTCTRLB, (uint8_t*)"\x00\x00", 2);
  _write(_PWCTRL1, (uint8_t*)"\x23", 1);
  _write(_PWCTRL2, (uint8_t*)"\x10", 1);
  _write(_VMCTRL1, (uint8_t*)"\x3e\x28", 2);
  _write(_VMCTRL2, (uint8_t*)"\x86", 1);

  _write(_PIXSET, (uint8_t*)"\x55", 1);
  _write(_FRMCTR1, (uint8_t*)"\x00\x18", 2);
  _write(_DISCTRL, (uint8_t*)"\x08\x82\x27", 3);
  _write(_ENA3G, (uint8_t*)"\x00", 1);
  _write(_GAMSET, (uint8_t*)"\x01", 1);
  _write(_PGAMCTRL, (uint8_t*)"\x0f\x31\x2b\x0c\x0e\x08\x4e\xf1\x37\x07\x10\x03\x0e\x09\x00", 15);
  _write(_NGAMCTRL, (uint8_t*)"\x00\x0e\x14\x03\x11\x07\x31\xc1\x48\x08\x0f\x0c\x31\x36\x0f", 15);

  // Exit Sleep
  _write(_SLPOUT, NULL, 0);

  sleep_ms(120);

  // Display on
  _write(_DISPON, NULL, 0);
}

void ILI934X::configure_ili9341_2()
{
  _write(0xEF, (uint8_t*)"\x03\x80\x02", 3);
  _write(0xCF, (uint8_t*)"\x00\xC1\x30", 3);
  _write(0xED, (uint8_t*)"\x64\x03\x12\x81", 4);
  _write(0xE8, (uint8_t*)"\x85\x00\x78", 3);
  _write(0xCB, (uint8_t*)"\x39\x2C\x00\x34\x02", 5);
  _write(0xF7, (uint8_t*)"\x20", 1);
  _write(0xEA, (uint8_t*)"\x00\x00", 2);

  // Power control
  _write(_PWCTRL1, (uint8_t*)"\x23, 1");

  // Power control
  _write(_PWCTRL2, (uint8_t*)"\x10, 1");

  // VCM control
  _write(_VMCTRL1, (uint8_t*)"\x3e\x28", 2);

  // VCM control2
  _write(_VMCTRL2, (uint8_t*)"\x86", 1);

  // Memory Access Control
  _write(_MADCTL, (uint8_t*)"\x40", 1);
  _write(_PIXSET, (uint8_t*)"\x55", 1);
  _write(_FRMCTR1, (uint8_t*)"\x00\x13", 2);

  // Display Function Control
  _write(_DISCTRL, (uint8_t*)"\x08\x82\x27", 3);

  // 3Gamma Function Disable
  _write(0xF2, (uint8_t*)"\x00", 1);

  // Gamma curve selected
  _write(_GAMSET, (uint8_t*)"\x01", 1);

  // Set Gamma
  _write(_PGAMCTRL, (uint8_t*)"\x0F\x31\x2B\x0C\x0E\x08\x4E\xF1\x37\x07\x10\x03\x0E\x09\x00", 15);

  // Set Gamma
  _write(_NGAMCTRL, (uint8_t*)"\x00\x0E\x14\x03\x11\x07\x31\xC1\x48\x08\x0F\x0C\x31\x36\x0F", 15);

  // Exit Sleep
  _write(_SLPOUT, NULL, 0);

  sleep_ms(120);

  // Display on
  _write(_DISPON, NULL, 0);
}

void ILI934X::init(ILI934X_ROTATION rotation, bool invert_colours, bool invert_display,
                   e_display_type display_type, uint32_t tft_baud_rate)
{
  m_tft_baud_rate = tft_baud_rate;
  spi_set_baudrate(m_spi, m_tft_baud_rate);
  sleep_ms(5);
  _write(_SWRST, NULL, 0);
  sleep_ms(150);

  if (display_type == ILI9341) {
    configure_ili934x();
  } else if (display_type == ST7796) {
    configure_st7796();
  } else if (display_type == ILI9488) {
    configure_ili9488();
  } else if (display_type == ILI9341_2) {
    configure_ili9341_2();
  }

  if (invert_display) {
    _write(_INVON);
  } else {
    _write(_INVOFF);
  }

  _setRotation(rotation, invert_colours);
  _display_type = display_type;
}

void ILI934X::powerOn(bool power_on)
{
  if (power_on) {
    _write(_DISPON, NULL, 0);
  } else {
    _write(_DISPOFF, NULL, 0);
  }
}

void ILI934X::_setRotation(ILI934X_ROTATION rotation, bool invert_colours)
{

  m_rotation = rotation;
  uint8_t colour_flags = invert_colours ? MADCTL_BGR : MADCTL_RGB;
  uint8_t mode = MADCTL_MX | colour_flags;
  switch (rotation) {
  case R0DEG:
    mode = MADCTL_MX | colour_flags;
    this->_width = this->m_init_width;
    this->_height = this->m_init_height;
    break;
  case R90DEG:
    mode = MADCTL_MV | colour_flags;
    this->_width = this->m_init_width;
    this->_height = this->m_init_height;
    break;
  case R180DEG:
    mode = MADCTL_MY | colour_flags;
    this->_width = this->m_init_width;
    this->_height = this->m_init_height;
    break;
  case R270DEG:
    mode = MADCTL_MY | MADCTL_MX | MADCTL_MV | colour_flags;
    this->_width = this->m_init_width;
    this->_height = this->m_init_height;
    break;
  case MIRRORED0DEG:
    mode = MADCTL_MY | MADCTL_MX | colour_flags;
    this->_width = this->m_init_width;
    this->_height = this->m_init_height;
    break;
  case MIRRORED90DEG:
    mode = MADCTL_MX | MADCTL_MV | colour_flags;
    this->_width = this->m_init_width;
    this->_height = this->m_init_height;
    break;
  case MIRRORED180DEG:
    mode = colour_flags;
    this->_width = this->m_init_width;
    this->_height = this->m_init_height;
    break;
  case MIRRORED270DEG:
    mode = MADCTL_MY | MADCTL_MV | colour_flags;
    this->_width = this->m_init_width;
    this->_height = this->m_init_height;
    break;
  }

  uint8_t buffer[1] = {mode};
  _write(_MADCTL, (uint8_t*)buffer, 1);
}

void ILI934X::writeHLine(uint16_t x, uint16_t y, uint16_t w, const uint16_t line[])
{
  _writeBlock(x, y, x + w - 1, y);
  _writePixels(line, w);
}
void ILI934X::writeVLine(uint16_t x, uint16_t y, uint16_t h, const uint16_t line[])
{
  _writeBlock(x, y, x, y + h - 1);
  _writePixels(line, h);
}

void ILI934X::setPixel(uint16_t x, uint16_t y, uint16_t colour)
{
  if (x < 0 || x >= _width || y < 0 || y >= _height)
    return;

  uint16_t buffer[1];
  buffer[0] = colour;

  _writeBlock(x, y, x, y);
  _writePixels(buffer, 1);
}

void ILI934X::fillRect(uint16_t x, uint16_t y, uint16_t h, uint16_t w, uint16_t colour)
{
  uint16_t _x = MIN(_width - 1, MAX(0, x));
  uint16_t _y = MIN(_height - 1, MAX(0, y));
  uint16_t _w = MIN(_width - x, MAX(1, w));
  uint16_t _h = MIN(_height - y, MAX(1, h));

  uint16_t buffer[_MAX_CHUNK_SIZE];
  for (int x = 0; x < _MAX_CHUNK_SIZE; x++) {
    buffer[x] = colour;
  }

  uint16_t totalChunks = ((uint32_t)w * h) / _MAX_CHUNK_SIZE;
  uint16_t remaining = ((uint32_t)w * h) % _MAX_CHUNK_SIZE;

  _writeBlock(_x, _y, _x + _w - 1, _y + _h - 1);

  for (uint16_t i = 0; i < totalChunks; i++) {
    _writePixels((uint16_t*)buffer, _MAX_CHUNK_SIZE);
  }

  if (remaining > 0) {
    _writePixels((uint16_t*)buffer, remaining);
  }
}

void ILI934X::drawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color)
{
  int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int err = dx + dy, e2;

  while (1) {
    setPixel(x0, y0, color);
    if (x0 == x1 && y0 == y1)
      break;
    e2 = 2 * err;
    if (e2 >= dy) {
      err += dy;
      x0 += sx;
    }
    if (e2 <= dx) {
      err += dx;
      y0 += sy;
    }
  }
}

static uint16_t alpha_blend(uint16_t bg, uint16_t fg, uint16_t alpha)
{
  if (alpha >= 256)
    return fg;

  fg = (fg >> 8) | (fg << 8);
  bg = (bg >> 8) | (bg << 8);

  uint16_t r_bg = (bg >> 11) & 0x1F; // Extract red (5 bits)
  uint16_t g_bg = (bg >> 5) & 0x3F;  // Extract green (6 bits)
  uint16_t b_bg = bg & 0x1F;         // Extract blue (5 bits)

  uint16_t r_fg = (fg >> 11) & 0x1F; // Extract red (5 bits)
  uint16_t g_fg = (fg >> 5) & 0x3F;  // Extract green (6 bits)
  uint16_t b_fg = fg & 0x1F;         // Extract blue (5 bits)

  const uint16_t not_alpha = 256 - alpha;
  r_bg = ((r_bg * not_alpha) + (r_fg * alpha)) >> 8;
  g_bg = ((g_bg * not_alpha) + (g_fg * alpha)) >> 8;
  b_bg = ((b_bg * not_alpha) + (b_fg * alpha)) >> 8;

  uint16_t result = (r_bg << 11) | (g_bg << 5) | b_bg;
  return (result >> 8) | (result << 8);
}

#ifdef SMOMOTH_FONTS

void ILI934X::drawChar(uint32_t x, uint32_t y, const uint8_t* font, char c, uint16_t fg,
                       uint16_t bg)
{

  const uint8_t font_height = font[0];
  const uint8_t font_width = font[1];
  const uint8_t font_space = font[2];
  const uint8_t first_char = font[3];
  const uint8_t last_char = font[4];
  const uint16_t bytes_per_char = (font_width * font_height + 4) / 8;
  if (c < first_char || c > last_char)
    return;

  uint8_t buffer_grayscale[font_height][font_width];
  uint16_t buffer[font_height][font_width + font_space];
  uint16_t font_index = ((c - first_char) * bytes_per_char) + 5u;
  uint8_t data = font[font_index++];
  uint8_t bits_left = 8;

  // initial grayscale buffer
  for (uint8_t xx = 0; xx < font_width; ++xx) {
    for (uint8_t yy = 0; yy < font_height; ++yy) {
      if (data & 0x01) {
        buffer_grayscale[yy][xx] = 0x0f;
      } else {
        buffer_grayscale[yy][xx] = 0x00;
      }
      data >>= 1;
      bits_left--;
      if (bits_left == 0) {
        data = font[font_index++];
        bits_left = 8;
      }
    }
  }

  // smooth font
  for (uint8_t xx = 0; xx < font_width; ++xx) {
    for (uint8_t yy = 0; yy < font_height; ++yy) {
      uint8_t C = buffer_grayscale[yy][xx];
      uint16_t weighting = 0;
      if (C) {
        weighting = 256;
      } else {
        uint8_t N = yy > 0 ? buffer_grayscale[yy - 1][xx] : 0;
        uint8_t S = yy < font_height - 1 ? buffer_grayscale[yy + 1][xx] : 0;
        uint8_t W = xx > 0 ? buffer_grayscale[yy][xx - 1] : 0;
        uint8_t E = xx < font_width - 1 ? buffer_grayscale[yy][xx + 1] : 0;
        if ((N && E) || (N && W) || (S && E) || (S && W)) {
          weighting = 96;
        }
      }
      buffer[yy][xx] = alpha_blend(bg, fg, weighting);
    }
  }

  // fill in font gap
  for (uint8_t xx = 0; xx < font_space; ++xx) {
    for (uint8_t yy = 0; yy < font_height; ++yy) {
      buffer[yy][font_width + xx] = bg;
    }
  }

  _writeBlock(x, y, x + font_width + font_space - 1, y + font_height - 1);
  _writePixels((uint16_t*)buffer, sizeof(buffer) / 2);
}

#else

void ILI934X::drawChar(uint32_t x, uint32_t y, const uint8_t* font, char c, uint16_t fg,
                       uint16_t bg)
{

  const uint8_t font_height = font[0];
  const uint8_t font_width = font[1];
  const uint8_t font_space = font[2];
  const uint8_t first_char = font[3];
  const uint8_t last_char = font[4];
  const uint16_t bytes_per_char = (font_width * font_height + 4) / 8;
  if (c < first_char || c > last_char)
    return;

  uint16_t buffer[font_height][font_width + font_space];
  uint16_t font_index = ((c - first_char) * bytes_per_char) + 5u;
  uint8_t data = font[font_index++];
  uint8_t bits_left = 8;

  for (uint8_t xx = 0; xx < font_width; ++xx) {
    for (uint8_t yy = 0; yy < font_height; ++yy) {
      if (data & 0x01) {
        buffer[yy][xx] = fg;
      } else {
        buffer[yy][xx] = bg;
      }
      data >>= 1;
      bits_left--;
      if (bits_left == 0) {
        data = font[font_index++];
        bits_left = 8;
      }
    }
  }
  for (uint8_t xx = 0; xx < font_space; ++xx) {
    for (uint8_t yy = 0; yy < font_height; ++yy) {
      buffer[yy][font_width + xx] = bg;
    }
  }

  _writeBlock(x, y, x + font_width + font_space - 1, y + font_height - 1);
  _writePixels((uint16_t*)buffer, sizeof(buffer) / 2);
}

#endif

void ILI934X::drawString(uint32_t x, uint32_t y, const uint8_t* font, const char* s, uint16_t fg,
                         uint16_t bg)
{
  const uint8_t font_width = font[1];
  const uint8_t font_space = font[2];
  for (int32_t x_n = x; *s; x_n += (font_width + font_space)) {
    drawChar(x_n, y, font, *(s++), fg, bg);
  }
}

void ILI934X::clear(uint16_t colour)
{
  fillRect(0, 0, _height, _width, colour);
}

void ILI934X::_write(uint8_t cmd, uint8_t* data, size_t dataLen)
{
#ifdef DEBUG_SPI
  Serial.print("command ");
  Serial.println(cmd, HEX);

  for (int idx = 0; idx < dataLen; idx++) {
    Serial.print("data ");
    Serial.println(data[idx], HEX);
  }
#endif

  gpio_put(m_dc, 0);
  gpio_put(m_cs, 0);
  sleep_us(1);

  // spi write
  uint8_t commandBuffer[1];
  commandBuffer[0] = cmd;
  spi_write_blocking(m_spi, commandBuffer, 1);

  // do stuff
  if (data != NULL) {
    _data(data, dataLen);
  } else {
    sleep_us(1);
    gpio_put(m_cs, 1);
  }
}

void ILI934X::_data(uint8_t* data, size_t dataLen)
{
  gpio_put(m_dc, 1);
  gpio_put(m_cs, 0);
  sleep_us(1);
  spi_write_blocking(m_spi, data, dataLen);
  sleep_us(1);
  gpio_put(m_cs, 1);
}

void ILI934X::_writePixels(const uint16_t* data, size_t numPixels)
{
  if (_display_type == ILI9488) {
    uint32_t pixelIndex = 0;
    while (numPixels) {
      const uint32_t chunkSize = std::min((uint32_t)numPixels, (uint32_t)256);
      uint8_t new_data[256][3];
      for (uint32_t idx = 0; idx < chunkSize; idx++) {
        uint16_t pixel = __builtin_bswap16(data[pixelIndex++]);
        new_data[idx][0] = (pixel & 0xf800) >> 8;
        new_data[idx][1] = (pixel & 0x07e0) >> 3;
        new_data[idx][2] = (pixel & 0x001F) << 3;
      }
      _data((uint8_t*)new_data, 3 * chunkSize);

      numPixels -= chunkSize;
    }
  } else {
    _data((uint8_t*)data, 2 * numPixels);
  }
}

void ILI934X::_writeBlock(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t* data,
                          size_t dataLen)
{

  uint16_t buffer[2];
  buffer[0] = __builtin_bswap16(x0);
  buffer[1] = __builtin_bswap16(x1);

  _write(_CASET, (uint8_t*)buffer, 4);

  buffer[0] = __builtin_bswap16(y0);
  buffer[1] = __builtin_bswap16(y1);

  _write(_PASET, (uint8_t*)buffer, 4);
  _write(_RAMWR, data, dataLen);
}

uint16_t ILI934X::colour565(uint8_t r, uint8_t g, uint8_t b)
{
  uint16_t val = (((r >> 3) & 0x1f) << 11) | (((g >> 2) & 0x3f) << 5) | ((b >> 3) & 0x1f);
  return __builtin_bswap16(val);
}

void ILI934X::writeImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t* data)
{
  _writeBlock(x, y, x + w - 1, y + h - 1);
  _writePixels(data, w * h);
}

void ILI934X::fillCircle(uint16_t xc, uint16_t yc, uint16_t r, uint16_t colour)
{
  int x = 0;
  int y = r;
  int d = 3 - 2 * r;

  while (x <= y) {
    // Draw horizontal spans between symmetric points
    drawFastHline(xc - x, xc + x, yc + y, colour);
    drawFastHline(xc - x, xc + x, yc - y, colour);
    drawFastHline(xc - y, xc + y, yc + x, colour);
    drawFastHline(xc - y, xc + y, yc - x, colour);

    if (d < 0) {
      d = d + 4 * x + 6;
    } else {
      d = d + 4 * (x - y) + 10;
      y--;
    }
    x++;
  }
}

void ILI934X::drawCircle(uint16_t xc, uint16_t yc, uint16_t radius, uint16_t colour)
{
  int16_t x = 0, y = radius;
  int16_t d = 1 - radius;

  while (x <= y) {
    setPixel(xc + x, yc + y, colour);
    setPixel(xc - x, yc + y, colour);
    setPixel(xc + x, yc - y, colour);
    setPixel(xc - x, yc - y, colour);
    setPixel(xc + y, yc + x, colour);
    setPixel(xc - y, yc + x, colour);
    setPixel(xc + y, yc - x, colour);
    setPixel(xc - y, yc - x, colour);

    x++;
    if (d < 0) {
      d += 2 * x + 1;
    } else {
      y--;
      d += 2 * (x - y) + 1;
    }
  }
}

void ILI934X::drawRect(uint16_t x, uint16_t y, uint16_t h, uint16_t w, uint16_t colour)
{
  fillRect(x, y, 1, w, colour);
  fillRect(x, y + h - 1, 1, w, colour);
  fillRect(x, y, h, 1, colour);
  fillRect(x + w - 1, y, h, 1, colour);
}

void ILI934X::drawEllipse(int xc, int yc, int rx, int ry, uint16_t colour)
{
  int x = 0;
  int y = ry;
  int rx2 = rx * rx;
  int ry2 = ry * ry;
  int two_rx2 = 2 * rx2;
  int two_ry2 = 2 * ry2;
  int px = 0;
  int py = two_rx2 * y;

  // Region 1
  int p1 = ry2 - (rx2 * ry) + (0.25 * rx2);
  while (px < py) {
    // Draw symmetry points
    setPixel(xc + x, yc + y, colour);
    setPixel(xc - x, yc + y, colour);
    setPixel(xc + x, yc - y, colour);
    setPixel(xc - x, yc - y, colour);

    x++;
    px += two_ry2;
    if (p1 < 0) {
      p1 += ry2 + px;
    } else {
      y--;
      py -= two_rx2;
      p1 += ry2 + px - py;
    }
  }

  // Region 2
  int p2 = ry2 * (x + 0.5f) * (x + 0.5f) + rx2 * (y - 1) * (y - 1) - rx2 * ry2;
  while (y >= 0) {
    setPixel(xc + x, yc + y, colour);
    setPixel(xc - x, yc + y, colour);
    setPixel(xc + x, yc - y, colour);
    setPixel(xc - x, yc - y, colour);

    y--;
    py -= two_rx2;
    if (p2 > 0) {
      p2 += rx2 - py;
    } else {
      x++;
      px += two_ry2;
      p2 += rx2 - py + px;
    }
  }
}

void ILI934X::drawFastHline(int x0, int x1, int y, uint16_t colour)
{
  fillRect(std::min(x0, x1), y, 1, std::max(x0, x1) - std::min(x0, x1) + 1, colour);
}

void ILI934X::drawFastVline(int x, int y0, int y1, uint16_t colour)
{
  fillRect(x, std::min(y0, y1), std::max(y0, y1) - std::min(y0, y1) + 1, 1, colour);
}

void ILI934X::fillEllipse(int xc, int yc, int rx, int ry, uint16_t colour)
{
  int x = 0;
  int y = ry;
  int rx2 = rx * rx;
  int ry2 = ry * ry;
  int two_rx2 = 2 * rx2;
  int two_ry2 = 2 * ry2;
  int px = 0;
  int py = two_rx2 * y;

  // Region 1
  int p1 = ry2 - (rx2 * ry) + (0.25 * rx2);
  while (px < py) {
    drawFastHline(xc - x, xc + x, yc + y, colour);
    drawFastHline(xc - x, xc + x, yc - y, colour);

    x++;
    px += two_ry2;
    if (p1 < 0) {
      p1 += ry2 + px;
    } else {
      y--;
      py -= two_rx2;
      p1 += ry2 + px - py;
    }
  }

  // Region 2
  int p2 = ry2 * (x + 0.5f) * (x + 0.5f) + rx2 * (y - 1) * (y - 1) - rx2 * ry2;
  while (y >= 0) {
    drawFastHline(xc - x, xc + x, yc + y, colour);
    drawFastHline(xc - x, xc + x, yc - y, colour);

    y--;
    py -= two_rx2;
    if (p2 > 0) {
      p2 += rx2 - py;
    } else {
      x++;
      px += two_ry2;
      p2 += rx2 - py + px;
    }
  }
}

void ILI934X::drawFilledCircleQuadrant(int xc, int yc, int r, int quadrant, uint16_t colour)
{
  int x = 0;
  int y = r;
  int d = 3 - 2 * r;

  while (x <= y) {
    // Draw only one quarter depending on quadrant
    switch (quadrant) {
    case 0: // Top-left
      fillRect(xc - y, yc - x, 1, y + 1, colour);
      fillRect(xc - x, yc - y, 1, x + 1, colour);
      break;
    case 1: // Top-right
      fillRect(xc, yc - x, 1, y + 1, colour);
      fillRect(xc, yc - y, 1, x + 1, colour);
      break;
    case 2: // Bottom-left
      fillRect(xc - y, yc + x, 1, y + 1, colour);
      fillRect(xc - x, yc + y, 1, x + 1, colour);
      break;
    case 3: // Bottom-right
      fillRect(xc, yc + x, 1, y + 1, colour);
      fillRect(xc, yc + y, 1, x + 1, colour);
      break;
    }

    if (d < 0) {
      d += 4 * x + 6;
    } else {
      d += 4 * (x - y) + 10;
      y--;
    }
    x++;
  }
}

void ILI934X::fillRoundedRect(int x, int y, int h, int w, int r, uint16_t colour)
{
  // Sanity check: radius can't be more than half width/height
  if (r > w / 2)
    r = w / 2;
  if (r > h / 2)
    r = h / 2;

  // 1. Draw center rectangle (main body)
  fillRect(x + r, y, r, w - (2 * r), colour);
  fillRect(x, y + r, h - (2 * r), w, colour);
  fillRect(x + r, y + h - r, r, w - (2 * r), colour);

  // 2. Draw rounded corners (quarter circles)
  drawFilledCircleQuadrant(x + r, y + r, r, 0, colour);                 // Top-left
  drawFilledCircleQuadrant(x + w - r - 1, y + r, r, 1, colour);         // Top-right
  drawFilledCircleQuadrant(x + r, y + h - r - 1, r, 2, colour);         // Bottom-left
  drawFilledCircleQuadrant(x + w - r - 1, y + h - r - 1, r, 3, colour); // Bottom-right
}

void ILI934X::drawCircleQuadrant(int xc, int yc, int r, int quadrant, uint16_t colour)
{
  int x = 0;
  int y = r;
  int d = 3 - 2 * r;

  while (x <= y) {
    switch (quadrant) {
    case 0: // Top-left
      setPixel(xc - x, yc - y, colour);
      setPixel(xc - y, yc - x, colour);
      break;
    case 1: // Top-right
      setPixel(xc + x, yc - y, colour);
      setPixel(xc + y, yc - x, colour);
      break;
    case 2: // Bottom-left
      setPixel(xc - x, yc + y, colour);
      setPixel(xc - y, yc + x, colour);
      break;
    case 3: // Bottom-right
      setPixel(xc + x, yc + y, colour);
      setPixel(xc + y, yc + x, colour);
      break;
    }

    if (d < 0) {
      d += 4 * x + 6;
    } else {
      d += 4 * (x - y) + 10;
      y--;
    }
    x++;
  }
}

void ILI934X::drawRoundedRect(int x, int y, int h, int w, int r, uint16_t colour)
{
  // Limit radius
  if (r > w / 2)
    r = w / 2;
  if (r > h / 2)
    r = h / 2;

  // 1. Straight sides
  fillRect(x + r, y, 1, w - 2 * r, colour);         // Top
  fillRect(x + r, y + h - 1, 1, w - 2 * r, colour); // Bottom
  fillRect(x, y + r, h - 2 * r, 1, colour);         // Left
  fillRect(x + w - 1, y + r, h - 2 * r, 1, colour); // Right

  // 2. Rounded corners
  drawCircleQuadrant(x + r, y + r, r, 0, colour);                 // Top-left
  drawCircleQuadrant(x + w - r - 1, y + r, r, 1, colour);         // Top-right
  drawCircleQuadrant(x + r, y + h - r - 1, r, 2, colour);         // Bottom-left
  drawCircleQuadrant(x + w - r - 1, y + h - r - 1, r, 3, colour); // Bottom-right
}

void ILI934X::drawTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t colour)
{
  drawLine(x0, y0, x1, y1, colour);
  drawLine(x1, y1, x2, y2, colour);
  drawLine(x2, y2, x0, y0, colour);
}

void ILI934X::fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t colour)
{
  // Sort the vertices by y-coordinate ascending (y0 <= y1 <= y2)
  if (y0 > y1) {
    int t;
    t = y0;
    y0 = y1;
    y1 = t;
    t = x0;
    x0 = x1;
    x1 = t;
  }
  if (y1 > y2) {
    int t;
    t = y1;
    y1 = y2;
    y2 = t;
    t = x1;
    x1 = x2;
    x2 = t;
  }
  if (y0 > y1) {
    int t;
    t = y0;
    y0 = y1;
    y1 = t;
    t = x0;
    x0 = x1;
    x1 = t;
  }

  int total_height = y2 - y0;

  for (int y = y0; y <= y2; y++) {
    int second_half = y > y1 || y1 == y0;
    int segment_height = second_half ? y2 - y1 : y1 - y0;
    if (segment_height == 0)
      continue;

    float alpha = (float)(y - y0) / total_height;
    float beta = (float)(y - (second_half ? y1 : y0)) / segment_height;

    int ax = x0 + (x2 - x0) * alpha;
    int bx = second_half ? x1 + (x2 - x1) * beta : x0 + (x1 - x0) * beta;

    if (ax > bx) {
      int t = ax;
      ax = bx;
      bx = t;
    }
    drawFastHline(ax, bx, y, colour);
  }
}

void ILI934X::init_touchscreen(uint8_t cs, uint32_t touchscreen_baud_rate)
{
  m_touch_baud_rate = touchscreen_baud_rate;
  m_cs_touch = cs;
  m_touch_enabled = true;
}

static inline uint16_t spi_transfer16(spi_inst_t* spi, uint16_t tx)
{
  uint8_t txbuf[2] = {(uint8_t)(tx >> 8), (uint8_t)(tx & 0xFF)};
  uint8_t rxbuf[2];

  spi_write_read_blocking(spi, txbuf, rxbuf, 2);

  return ((uint16_t)rxbuf[0] << 8) | rxbuf[1];
}

static int16_t besttwoavg(int16_t x, int16_t y, int16_t z)
{
  int16_t da, db, dc;
  int16_t reta = 0;
  if (x > y)
    da = x - y;
  else
    da = y - x;
  if (x > z)
    db = x - z;
  else
    db = z - x;
  if (z > y)
    dc = z - y;
  else
    dc = y - z;

  if (da <= db && da <= dc)
    reta = (x + y) >> 1;
  else if (db <= da && db <= dc)
    reta = (x + z) >> 1;
  else
    reta = (y + z) >> 1;
  return (reta);
}

bool ILI934X::touched()
{
  if (!m_touch_enabled)
    return false;
  update();
  return (m_zraw >= Z_THRESHOLD);
}

void ILI934X::getPoint(int16_t& x, int16_t& y, int16_t& z)
{
  if (!m_touch_enabled) {
    x = y = z = 0;
    return;
  }
  update();

  x = (m_ax * m_xraw + m_bx);
  if (x < 0)
    x = 0;
  if (x > _width - 1)
    x = _width - 1;

  y = (m_ay * m_yraw + m_by);
  if (y < 0)
    y = 0;
  if (y > _height - 1)
    y = _height - 1;

  z = m_zraw;
}

bool ILI934X::touch_calibrate()
{
  static const int cal_points[4][2] = {
      {20, 20}, {_width - 20, 20}, {20, _height - 20}, {_width - 20, _height - 20}};

  float raw_x[4], raw_y[4];
  clear(COLOUR_BLACK);
  for (int i = 0; i < 4; i++) {
    drawCircle(cal_points[i][0], cal_points[i][1], 3, COLOUR_WHITE);

    while (!touched())
      tight_loop_contents();

    float x_total = 0.0f;
    float y_total = 0.0f;
    int32_t num_meas = 0;
    while (num_meas < 1024.0f) {
      if (touched()) {
        x_total += m_xraw;
        y_total += m_yraw;
        num_meas += 1;
      }
    }
    raw_x[i] = x_total / 1024.0f;
    raw_y[i] = y_total / 1024.0f;

    drawCircle(cal_points[i][0], cal_points[i][1], 3, COLOUR_GREEN);

    while (touched())
      tight_loop_contents();

    sleep_ms(200);
  }

  // Compute scale + offset
  m_ax = (float)(cal_points[1][0] - cal_points[0][0]) / (float)(raw_x[1] - raw_x[0]);
  m_bx = cal_points[0][0] - m_ax * raw_x[0];

  m_ay = (float)(cal_points[2][1] - cal_points[0][1]) / (float)(raw_y[2] - raw_y[0]);
  m_by = cal_points[0][1] - m_ay * raw_y[0];

  return true;
}

void ILI934X::update()
{

  int16_t data[6];
  int32_t z;

  spi_set_baudrate(m_spi, m_touch_baud_rate);
  gpio_put(m_cs_touch, 0);

  uint8_t cmd = 0xB1;
  uint8_t rx;
  spi_write_read_blocking(m_spi, &cmd, &rx, 1);
  int16_t z1 = spi_transfer16(m_spi, 0xC1) >> 3;

  z = z1 + 4095;

  // int16_t z2 = _pspi->transfer16(0x91 /* X */) >> 3;
  int16_t z2 = spi_transfer16(m_spi, 0x91) >> 3;
  z -= z2;

  if (z >= Z_THRESHOLD) {
    // dummy X measure, 1st is always noisy
    spi_transfer16(m_spi, 0x91);
    sleep_us(20);

    data[0] = spi_transfer16(m_spi, 0xD1 /* Y */) >> 3;
    data[1] = spi_transfer16(m_spi, 0x91 /* X */) >> 3;
    data[2] = spi_transfer16(m_spi, 0xD1 /* Y */) >> 3;
    data[3] = spi_transfer16(m_spi, 0x91 /* X */) >> 3;
  } else {
    data[0] = data[1] = data[2] = data[3] = 0;
  }

  // Last Y touch power down
  data[4] = spi_transfer16(m_spi, 0xD0 /* Y */) >> 3;
  data[5] = spi_transfer16(m_spi, 0x0000) >> 3;

  gpio_put(m_cs_touch, 1);
  spi_set_baudrate(m_spi, m_tft_baud_rate);

  if (z < 0)
    z = 0;
  if (z < Z_THRESHOLD) {
    m_zraw = 0;
    return;
  }
  m_zraw = z;

  // Average pair with least distance between each measured x then y
  int16_t x = besttwoavg(data[0], data[2], data[4]);
  int16_t y = besttwoavg(data[1], data[3], data[5]);

  m_xraw = x;
  m_yraw = y;
  return;

  if (z >= Z_THRESHOLD) {
    switch (m_rotation) {
    case R0DEG:
      m_xraw = y;
      m_yraw = 4095 - x;
      break;
    case R90DEG:
      m_xraw = 4095 - x;
      m_yraw = 4095 - y;
      break;
    case R180DEG:
      m_xraw = 4095 - y;
      m_yraw = x;
      break;
    case R270DEG:
      m_xraw = x;
      m_yraw = y;
      break;
    case MIRRORED0DEG:
      m_xraw = y;
      m_yraw = x;
      break;
    case MIRRORED90DEG:
      m_xraw = 4095 - x;
      m_yraw = y;
      break;
    case MIRRORED180DEG:
      m_xraw = 4095 - y;
      m_yraw = 4095 - x;
      break;
    case MIRRORED270DEG:
      m_xraw = x;
      m_yraw = 4095 - y;
      break;
    }
  }
}

#endif
