/*****************************************************************//**
 * @file timer_core.cpp
 *
 * @brief implementation of various video core classes
 *
 * @author p chu
 * @version v1.0: initial release
 ********************************************************************/

#include "vga_core.h"

/**********************************************************************
 * General purpose video core methods
 *********************************************************************/
GpvCore::GpvCore(uint32_t core_base_addr) {
   base_addr = core_base_addr;
}
GpvCore::~GpvCore() {
}

void GpvCore::wr_mem(int addr, uint32_t data) {
   io_write(base_addr, addr, data);
}

void GpvCore::bypass(int by) {
   io_write(base_addr, BYPASS_REG, (uint32_t ) by);
}

/**********************************************************************
 * Sprite core methods
 *********************************************************************/
SpriteCore::SpriteCore(uint32_t core_base_addr, int sprite_size) {
   base_addr = core_base_addr;
   size = sprite_size;
}
SpriteCore::~SpriteCore() {
}

void SpriteCore::wr_mem(int addr, uint32_t color) {
   io_write(base_addr, addr, color);
}

void SpriteCore::bypass(int by) {
   io_write(base_addr, BYPASS_REG, (uint32_t ) by);
}

void SpriteCore::move_xy(int x, int y) {
   io_write(base_addr, X_REG, x);
   io_write(base_addr, Y_REG, y);
   return;
}

void SpriteCore::wr_ctrl(int32_t cmd) {
   io_write(base_addr, SPRITE_CTRL_REG, cmd);
}


/**********************************************************************
 * OSD core methods
 *********************************************************************/
OsdCore::OsdCore(uint32_t core_base_addr) {
   base_addr = core_base_addr;
   set_color(0x0f0, CHROMA_KEY_COLOR);  // green on black
}
OsdCore::~OsdCore() {
}
// not used

void OsdCore::set_color(uint32_t fg_color, uint32_t bg_color) {
   io_write(base_addr, FG_CLR_REG, fg_color);
   io_write(base_addr, BG_CLR_REG, bg_color);
}

void OsdCore::wr_char(uint8_t x, uint8_t y, char ch, int reverse) {
   uint32_t ch_offset;
   uint32_t data;

   ch_offset = (y << 7) + (x & 0x07f);   // offset is concatenation of y and x
   if (reverse == 1)
      data = (uint32_t)(ch | 0x80);
   else
      data = (uint32_t) ch;
   io_write(base_addr, ch_offset, data);
   return;
}

void OsdCore::clr_screen() {
   int x, y;

   for (x = 0; x < CHAR_X_MAX; x++)
      for (y = 0; y < CHAR_Y_MAX; y++) {
         wr_char(x, y, NULL_CHAR);
      }
   return;
}

void OsdCore::bypass(int by) {
   io_write(base_addr, BYPASS_REG, (uint32_t ) by);
}

/**********************************************************************
 * FrameCore core methods
 *********************************************************************/
FrameCore::FrameCore(uint32_t frame_base_addr) {
   base_addr = frame_base_addr;
}
FrameCore::~FrameCore() {
}
// not used
#ifndef _swap_int16_t
#define _swap_int16_t(a, b)                                                    \
  {                                                                            \
    int16_t t = a;                                                             \
    a = b;                                                                     \
    b = t;                                                                     \
  }
#endif
void FrameCore::wr_pix(int x, int y, int color) {
   uint32_t pix_offset;

   pix_offset = HMAX * y + x;
   io_write(base_addr, pix_offset, color);
   return;
}

void FrameCore::clr_screen(int color) {
   int x, y;

   for (x = 0; x < HMAX; x++)
      for (y = 0; y < VMAX; y++) {
         wr_pix(x, y, color);
      }
   return;
}

void FrameCore::bypass(int by) {
   io_write(base_addr, BYPASS_REG, (uint32_t ) by);
}

// from AdaFruit
void FrameCore::plot_line(int x0, int y0, int x1, int y1, int color) {
   int dx, dy;
   int err, ystep, steep;

   if (x0 > x1) {
      swap(x0, x1);
      swap(y0, y1);
   }
   // slope is high
   steep = (abs(y1 - y0) > abs(x1 - x0)) ? 1 : 0;
   if (steep) {
      swap(x0, y0);
      swap(x1, y1);
   }
   dx = x1 - x0;
   dy = abs(y1 - y0);
   err = dx / 2;
   if (y0 < y1) {
      ystep = 1;
   } else {
      ystep = -1;
   }
   for (; x0 <= x1; x0++) {
      if (steep) {
         wr_pix(y0, x0, color);
      } else {
         wr_pix(x0, y0, color);
      }
      err = err - dy;
      if (err < 0) {
         y0 = y0 + ystep;
         err = err + dx;
      }
   }
}

void FrameCore::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
//   startWrite();
  writeFastHLine(x, y, w, color);
  writeFastHLine(x, y + h - 1, w, color);
  writeFastVLine(x, y, h, color);
  writeFastVLine(x + w - 1, y, h, color);
//   endWrite();
}

void FrameCore::fillRect(int16_t x, int16_t y, int16_t w, int16_t h,
                            uint16_t color) {
//   startWrite();
  for (int16_t i = x; i < x + w; i++) {
    writeFastVLine(i, y, h, color);
  }
//   endWrite();
}

void FrameCore::drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
   #if defined(ESP8266)
   yield();
   #endif
   int16_t f = 1 - r;
   int16_t ddF_x = 1;
   int16_t ddF_y = -2 * r;
   int16_t x = 0;
   int16_t y = r;

   // startWrite();
   wr_pix(x0, y0 + r, color);
   wr_pix(x0, y0 - r, color);
   wr_pix(x0 + r, y0, color);
   wr_pix(x0 - r, y0, color);

   while (x < y) {
      if (f >= 0) {
         y--;
         ddF_y += 2;
         f += ddF_y;
      }
      x++;
      ddF_x += 2;
      f += ddF_x;

      wr_pix(x0 + x, y0 + y, color);
      wr_pix(x0 - x, y0 + y, color);
      wr_pix(x0 + x, y0 - y, color);
      wr_pix(x0 - x, y0 - y, color);
      wr_pix(x0 + y, y0 + x, color);
      wr_pix(x0 - y, y0 + x, color);
      wr_pix(x0 + y, y0 - x, color);
      wr_pix(x0 - y, y0 - x, color);
   }
   // endWrite();
}

void FrameCore::fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
//   startWrite();
  writeFastVLine(x0, y0 - r, 2 * r + 1, color);
  fillCircleHelper(x0, y0, r, 3, 0, color);
//   endWrite();
}

void FrameCore::fillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t corners, int16_t delta, uint16_t color) {

  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;
  int16_t px = x;
  int16_t py = y;

  delta++; // Avoid some +1's in the loop

  while (x < y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
    // These checks avoid double-drawing certain lines, important
    // for the SSD1306 library which has an INVERT drawing mode.
    if (x < (y + 1)) {
      if (corners & 1)
        writeFastVLine(x0 + x, y0 - y, 2 * y + delta, color);
      if (corners & 2)
        writeFastVLine(x0 - x, y0 - y, 2 * y + delta, color);
    }
    if (y != py) {
      if (corners & 1)
        writeFastVLine(x0 + py, y0 - px, 2 * px + delta, color);
      if (corners & 2)
        writeFastVLine(x0 - py, y0 - px, 2 * px + delta, color);
      py = y;
    }
    px = x;
  }
}

void FrameCore::writeFastHLine(int16_t x, int16_t y, int16_t w,
                                  uint16_t color) {
  // Overwrite in subclasses if startWrite is defined!
  // Example: writeLine(x, y, x+w-1, y, color);
  // or writeFillRect(x, y, w, 1, color);
  drawFastHLine(x, y, w, color);
}

void FrameCore::drawFastHLine(int16_t x, int16_t y, int16_t w,
                                 uint16_t color) {
//   startWrite();
  writeLine(x, y, x + w - 1, y, color);
//   endWrite();
}

void FrameCore::writeFastVLine(int16_t x, int16_t y, int16_t h,
                                  uint16_t color) {
  // Overwrite in subclasses if startWrite is defined!
  // Can be just writeLine(x, y, x, y+h-1, color);
  // or writeFillRect(x, y, 1, h, color);
  drawFastVLine(x, y, h, color);
}

void FrameCore::drawFastVLine(int16_t x, int16_t y, int16_t h,
                                 uint16_t color) {
//   startWrite();
  writeLine(x, y, x, y + h - 1, color);
//   endWrite();
}

void FrameCore::writeLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                             uint16_t color) {
#if defined(ESP8266)
  yield();
#endif
  int16_t steep = abs(y1 - y0) > abs(x1 - x0);
  if (steep) {
    _swap_int16_t(x0, y0);
    _swap_int16_t(x1, y1);
  }

  if (x0 > x1) {
    _swap_int16_t(x0, x1);
    _swap_int16_t(y0, y1);
  }

  int16_t dx, dy;
  dx = x1 - x0;
  dy = abs(y1 - y0);

  int16_t err = dx / 2;
  int16_t ystep;

  if (y0 < y1) {
    ystep = 1;
  } else {
    ystep = -1;
  }

  for (; x0 <= x1; x0++) {
    if (steep) {
      wr_pix(y0, x0, color);
    } else {
      wr_pix(x0, y0, color);
    }
    err -= dy;
    if (err < 0) {
      y0 += ystep;
      err += dx;
    }
  }
}


void FrameCore::swap(int &a, int &b) {
   int tmp;

   tmp = a;
   a = b;
   b = tmp;
}

