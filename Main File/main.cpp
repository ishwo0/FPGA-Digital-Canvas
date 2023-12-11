/*****************************************************************//**
 * @file main_video_test.cpp
 *
 * @brief Basic test of 4 basic i/o cores
 *
 * @author p chu
 * @version v1.0: initial release
 *********************************************************************/

//#define _DEBUG
#include "chu_init.h"
#include "gpio_cores.h"
#include "vga_core.h"
#include "sseg_core.h"
#include "xadc_core.h"
#include "ps2_core.h"
#include "spi_core.h"
#include <cmath>
#include <cstdlib>
#include <ctime>

void test_start(GpoCore *led_p) {
   int i;

   for (i = 0; i < 20; i++) {
      led_p->write(0xff00);
      sleep_ms(50);
      led_p->write(0x0000);
      sleep_ms(50);
   }
}

/**
 * check bar generator core
 * @param bar_p pointer to Gpv instance
 */
void bar_check(GpvCore *bar_p) {
   bar_p->bypass(0);
   sleep_ms(3000);
}

/**
 * check color-to-grayscale core
 * @param gray_p pointer to Gpv instance
 */
void gray_check(GpvCore *gray_p) {
   gray_p->bypass(0);
   sleep_ms(3000);
   gray_p->bypass(1);
}

/**
 * check osd core
 * @param osd_p pointer to osd instance
 */
void osd_check(OsdCore *osd_p) {
   osd_p->set_color(0x0f0, 0x001); // dark gray/green
   osd_p->bypass(0);
   osd_p->clr_screen();
   for (int i = 0; i < 64; i++) {
      osd_p->wr_char(8 + i, 20, i);
      osd_p->wr_char(8 + i, 21, 64 + i, 1);
      sleep_ms(100);
   }
   sleep_ms(3000);
}

/**
 * test frame buffer core
 * @param frame_p pointer to frame buffer instance
 */
void frame_check(FrameCore *frame_p) {
   int x, y, color;

   frame_p->bypass(0);
   for (int i = 0; i < 10; i++) {
      frame_p->clr_screen(0x008);  // dark green
      for (int j = 0; j < 20; j++) {
         x = rand() % 640;
         y = rand() % 480;
         color = rand() % 512;
         frame_p->plot_line(400, 200, x, y, color);
      }
      sleep_ms(300);
   }
   sleep_ms(3000);
}

/**
 * test ghost sprite
 * @param ghost_p pointer to mouse sprite instance
 */
void mouse_check(SpriteCore *mouse_p) {
   int x, y;

   mouse_p->bypass(0);
   // clear top and bottom lines
   for (int i = 0; i < 32; i++) {
      mouse_p->wr_mem(i, 0);
      mouse_p->wr_mem(31 * 32 + i, 0);
   }

   // slowly move mouse pointer
   x = 0;
   y = 0;
   for (int i = 0; i < 80; i++) {
      mouse_p->move_xy(x, y);
      sleep_ms(50);
      x = x + 4;
      y = y + 3;
   }
   sleep_ms(3000);
   // load top and bottom rows
   for (int i = 0; i < 32; i++) {
      sleep_ms(20);
      mouse_p->wr_mem(i, 0x00f);
      mouse_p->wr_mem(31 * 32 + i, 0xf00);
   }
   sleep_ms(3000);
}

/**
 * test ghost sprite
 * @param ghost_p pointer to ghost sprite instance
 */
void ghost_check(SpriteCore *ghost_p) {
   int x, y;

   // slowly move mouse pointer
   ghost_p->bypass(0);
   ghost_p->wr_ctrl(0x1c);  //animation; blue ghost
   x = 0;
   y = 100;
   for (int i = 0; i < 156; i++) {
      ghost_p->move_xy(x, y);
      sleep_ms(100);
      x = x + 4;
      if (i == 80) {
         // change to red ghost half way
         ghost_p->wr_ctrl(0x04);
      }
   }
   sleep_ms(3000);
}



/* CANVAS FUNCTIONS */

double pot_value_color(XadcCore *adc_p) { // read potentiometer value for RGB LED spectrum color
   double reading;

   reading = adc_p->read_adc_in(0);
   return reading;

}

double pot_value_brush(XadcCore *adc_p) { // read potentiometer value for brush size
   double reading;

   reading = adc_p->read_adc_in(1);
   return reading;

}

double map(double mag, double in_min, double in_max, double out_min, double out_max) {

   return ((mag) - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

uint16_t map_rgb(double mag, double in_min, double in_max, double out_min, double out_max) {

   return (uint16_t)((mag - in_min) * (out_max - out_min) / (in_max - in_min) + out_min);
}


void spectrum(PwmCore *pwm_p, double duty_pot, double *r, double *g, double *b) {   // function for RGB LED spectrum
   double red, green, blue;

   pwm_p->set_freq(50); // set frequency to 50

	if (duty_pot >= 0.0 && duty_pot <= (1.0/6.0)) { // section 1 of pot
		pwm_p->set_duty(1.0, 2);   // red is maxed
      red = 0.999;

		pwm_p->set_duty(map(duty_pot, 0, (1.0/6.0), 0.0, 1.0), 1);  // green is varied (increasing)
      green = map(duty_pot, 0, (1.0/6.0), 0.03, 0.999);

		pwm_p->set_duty(0.0, 0);   // blue is off
      blue = 0.03;


	}
	else if(duty_pot > (1.0/6.0) && duty_pot <= (2.0/6.0)) { // section 2 of pot
		pwm_p->set_duty(map(duty_pot, (1.0/6.0), (2.0/6.0), 1.0, 0.0), 2);   // red is varied (decreasing)
      red = map(duty_pot, (1.0/6.0), (2.0/6.0), 0.999, 0.03);

		pwm_p->set_duty(1.0, 1);   // green is maxed
      green = 0.999;

		pwm_p->set_duty(0.0, 0);   // blue is off
      blue = 0.03;
      

		if(duty_pot >= 0.33) {
			pwm_p->set_duty(0.0, 2);   // smooth transitions
		}
	}
   else if(duty_pot > (2.0/6.0) && duty_pot <= (3.0/6.0)) { // section 3 of pot
		pwm_p->set_duty(0.0, 2);   // red is off
      red = 0.03;

		pwm_p->set_duty(1.0, 1);   // green is maxed
      green = 0.999;

		pwm_p->set_duty(map(duty_pot, (2.0/6.0), (3.0/6.0), 0.0, 1.0), 0);   // blue is varied (increasing)
      blue = map(duty_pot, (2.0/6.0), (3.0/6.0), 0.03, 0.999);

	}
   else if(duty_pot > (3.0/6.0) && duty_pot <= (4.0/6.0)) { // section 4 of pot
		pwm_p->set_duty(0.0, 2);   // red is off
      red = 0.03;

		pwm_p->set_duty(map(duty_pot, (3.0/6.0), (4.0/6.0), 1.0, 0.0), 1);   // green is varied (increasing)
      green = map(duty_pot, (3.0/6.0), (4.0/6.0), 0.999, 0.03);

		pwm_p->set_duty(1.0, 0);   // blue is maxed
      blue = 0.999;


		if(duty_pot >= 0.66) {
			pwm_p->set_duty(0.0, 1);   // smooth transitions
		}
	}
   else if(duty_pot > (4.0/6.0) && duty_pot <= (5.0/6.0)) { // section 5 of pot
		pwm_p->set_duty(map(duty_pot, (4.0/6.0), (5.0/6.0), 0.0, 1.0), 2);   // red is varied (increasing)
      red = map(duty_pot, (4.0/6.0), (5.0/6.0), 0.03, 0.999);

		pwm_p->set_duty(0.0, 1);   // green is off
      green = 0.03;

		pwm_p->set_duty(1.0, 0);   // blue is maxed
      blue = 0.999;

	}
   else if(duty_pot > (5.0/6.0) && duty_pot <= 9.999) {// section 6 of pot
      pwm_p->set_duty(1.0, 2);   // red is maxed
      red = 0.999;

      pwm_p->set_duty(0.0, 1);   // green is off
      green = 0.03;

      pwm_p->set_duty(map(duty_pot, (5.0/6.0), 1.0, 1.0, 0.0), 0);   // blue is varied (decreasing)
      blue = map(duty_pot, (5.0/6.0), 1.0, 0.999, 0.03);


	}


   // return RGB values
   *r = red;
   *g = green;
   *b = blue;

}


int ps2_init(Ps2Core *ps2_p) {   // initialize ps2 by getting device ID
   int id;
   uart.disp("\n\rPS2 device (1-keyboard / 2-mouse): ");
   id = ps2_p->init();
   uart.disp(id);
   uart.disp("\n\r");
   return id;
}

int canvas_mouse(Ps2Core *ps2_p, SsegCore *sseg_p, int id, int *btn_left, int *btn_right, int *xcord, int *ycord) {  // get mouse data
   int lbtn, rbtn, xmov, ymov;

      if(id == 2) {
         if (ps2_p->get_mouse_activity(&lbtn, &rbtn, &xmov, &ymov)) {   // if mouse data is valid
            // get the mouse data
            *btn_left = lbtn;
            *btn_right = rbtn;
            *xcord = xmov;
            *ycord = ymov;

            return 1;
         }   // end get_mouse_activitiy()
         else {
            return 0;
         }

      }
      else
         return 0;
   
   

}

void move_brush (SpriteCore *mouse_p, int btn_x, int btn_y, int *x, int *y) { // function to move brush around the screen based on the mouse data
   //
   int xx = *x;
   int yy = *y;
   mouse_p->bypass(0);  // do not bypass the mouse
   mouse_p->move_xy(xx, yy);  // move the mouse sprite to the x/y position

   // mouse movement based on sign of mouse data
   if(btn_x > 0) {
      xx = xx + 4;  // if moving mouse to the right (data x is positive), then cursor position x will increase 
   }
   else if (btn_x < 0) {
      xx = xx - 4;   // if moving mouse to left (data x is negative), then cursor position x will decrease
   }
   else {
      xx = xx; // dont move if mouse isnt moving
   }
   
   if(btn_y > 0) {
      yy = yy - 4;  // if moving mouse up (data y is positive), then cursor position y will decrease (since y decrements upward) 
   }
   else if (btn_y < 0) {
      yy = yy + 4;   // if moving mouse down (data y is negative), then cursor position y will increase (since y increments downward) 
   }
   else {
      yy = yy; // dont move if mouse isnt moving
   }


   // Checking if cursor exceeds the boundaries... if so, stop at boundary
   if(xx >= 630) {
      xx = 630;
   }

   if(xx <= 0) {
      xx = 0;
   }

   if(yy >= 450) {
      yy = 450;
   }

   if(yy <= 0) {
      yy = 0;
   }

   *x = xx;
   *y = yy;

}

void trademark(OsdCore *osd_p) {
   osd_p->set_color(0x111, 0x000);  // set foreground to black and background to transparent
   osd_p->bypass(0); // do not bypass OSD

   /* Display "PixelPoet", "v 1.1.0", and "Founded by Chia & Brian Corp" */
   osd_p->wr_char(5, 1, 80);   // P
   osd_p->wr_char(6, 1, 105);  // i
   osd_p->wr_char(7, 1, 120);  // x
   osd_p->wr_char(8, 1, 101);  // e
   osd_p->wr_char(9, 1, 108);  // l
   osd_p->wr_char(10, 1, 80);  // P
   osd_p->wr_char(11, 1, 111); // o
   osd_p->wr_char(12, 1, 101); // e
   osd_p->wr_char(13, 1, 116); // t

   
   osd_p->wr_char(70, 0, 118); // v
   // Space
   osd_p->wr_char(72, 0, 49); // 1
   osd_p->wr_char(73, 0, 46); // .
   osd_p->wr_char(74, 0, 49); // 1
   osd_p->wr_char(75, 0, 46); // .
   osd_p->wr_char(76, 0, 48); // 0


   osd_p->wr_char(54, 28, 70);  // F
   osd_p->wr_char(55, 28, 111); // o
   osd_p->wr_char(56, 28, 117); // u
   osd_p->wr_char(57, 28, 110); // n
   osd_p->wr_char(58, 28, 100); // d
   osd_p->wr_char(59, 28, 101); // e
   osd_p->wr_char(60, 28, 100); // d
   // Space
   osd_p->wr_char(62, 28, 98);  // b
   osd_p->wr_char(63, 28, 121); // y
   // Space
   osd_p->wr_char(65, 28, 84);  // T
   osd_p->wr_char(66, 28, 104); // h
   osd_p->wr_char(67, 28, 101); // e
   // Space
   osd_p->wr_char(69, 28, 66);  // B
   osd_p->wr_char(70, 28, 110); // n
   osd_p->wr_char(71, 28, 67);  // C
   // Space
   osd_p->wr_char(73, 28, 67);  // C
   osd_p->wr_char(74, 28, 111); // o
   osd_p->wr_char(75, 28, 114); // r
   osd_p->wr_char(76, 28, 112); // p

   // Clear button
   osd_p->wr_char(5, 24, 67);   // C
   osd_p->wr_char(6, 24, 108);  // l
   osd_p->wr_char(7, 24, 101);  // e
   osd_p->wr_char(8, 24, 97);   // a
   osd_p->wr_char(9, 24, 114);  // r

}

void welcome_msg_off(OsdCore *osd_p) {
   // turn off welcome message
   osd_p->wr_char(36, 10, 0);
   osd_p->wr_char(38, 10, 0);
   osd_p->wr_char(37, 10, 0);
   osd_p->wr_char(39, 10, 0);
   osd_p->wr_char(40, 10, 0);
   osd_p->wr_char(41, 10, 0);
   osd_p->wr_char(42, 10, 0);
   osd_p->wr_char(43, 10, 0);


   osd_p->wr_char(33, 11, 0);
   osd_p->wr_char(34, 11, 0);
   osd_p->wr_char(35, 11, 0);
   osd_p->wr_char(36, 11, 0);

   osd_p->wr_char(38, 11, 0);
   osd_p->wr_char(39, 11, 0);
   osd_p->wr_char(40, 11, 0);
   osd_p->wr_char(41, 11, 0);
   osd_p->wr_char(42, 11, 0);
   osd_p->wr_char(43, 11, 0);
   osd_p->wr_char(44, 11, 0);
   osd_p->wr_char(45, 11, 0);  
   osd_p->wr_char(46, 11, 0);
}

void welcome_msg(OsdCore *osd_p) {  // function to display a welcome message
   osd_p->set_color(0x111, 0x000);  // set foreground to black and background to transparent
   osd_p->bypass(0); // do not bypass OSD
   osd_p->clr_screen(); // clear screen initially
   
   trademark(osd_p); // display trademark

   /* Display "WELCOME!!" one letter at a time */
   osd_p->wr_char(36, 10, 87);   // W
   sleep_ms(200);
   osd_p->wr_char(37, 10, 69);   // E
   sleep_ms(200);
   osd_p->wr_char(38, 10, 76);   // L
   sleep_ms(200);
   osd_p->wr_char(39, 10, 67);   // C
   sleep_ms(200);
   osd_p->wr_char(40, 10, 79);   // O
   sleep_ms(200);
   osd_p->wr_char(41, 10, 77);   // M
   sleep_ms(200);
   osd_p->wr_char(42, 10, 69);   // E
   sleep_ms(200);
   osd_p->wr_char(43, 10, 19);   // !!
   sleep_ms(200);

   /* DRAW ANYTHING!*/
   osd_p->wr_char(33, 11, 68);   // D
   osd_p->wr_char(34, 11, 114);  // r
   osd_p->wr_char(35, 11, 97);   // a
   osd_p->wr_char(36, 11, 119);  // w
   /* Space */
   osd_p->wr_char(38, 11, 65);   // A
   osd_p->wr_char(39, 11, 110);  // n
   osd_p->wr_char(40, 11, 121);  // y
   osd_p->wr_char(41, 11, 116);  // t
   osd_p->wr_char(42, 11, 104);  // h
   osd_p->wr_char(43, 11, 105);  // i
   osd_p->wr_char(44, 11, 110);  // n
   osd_p->wr_char(45, 11, 103);  // g
   osd_p->wr_char(46, 11, 19);   // !!


   sleep_ms(200);

   /* Flash Welcome 3 times */
   for(int i = 0; i < 5; i++) {
      welcome_msg_off(osd_p);
      sleep_ms(500);
      /* WELCOME!! */
      osd_p->wr_char(36, 10, 87);   // W
      osd_p->wr_char(37, 10, 69);   // E
      osd_p->wr_char(38, 10, 76);   // L
      osd_p->wr_char(39, 10, 67);   // C
      osd_p->wr_char(40, 10, 79);   // O
      osd_p->wr_char(41, 10, 77);   // M
      osd_p->wr_char(42, 10, 69);   // E
      osd_p->wr_char(43, 10, 19);   // !!

      /* DRAW ANYTHING!*/
      osd_p->wr_char(33, 11, 68);   // D
      osd_p->wr_char(34, 11, 114);  // r
      osd_p->wr_char(35, 11, 97);   // a
      osd_p->wr_char(36, 11, 119);  // w
      /* Space */
      osd_p->wr_char(38, 11, 65);   // A
      osd_p->wr_char(39, 11, 110);  // n
      osd_p->wr_char(40, 11, 121);  // y
      osd_p->wr_char(41, 11, 116);  // t
      osd_p->wr_char(42, 11, 104);  // h
      osd_p->wr_char(43, 11, 105);  // i
      osd_p->wr_char(44, 11, 110);  // n
      osd_p->wr_char(45, 11, 103);  // g
      osd_p->wr_char(46, 11, 19);   // !!

      sleep_ms(500);


   }
   
   

}

void initialize_canvas(FrameCore *frame_p, int color0, int color1, int color2, int color3, int color4, int color5, int color6, int color7) { // function to initialize screen to white canvas
   frame_p->clr_screen(0xA8B);  // frame color
   frame_p->bypass(0);  // do not bypass frame buffer
   frame_p->fillRect(100, 70, 440, 340, 0xfff); // white canvas drawing area
   frame_p->drawRect(99, 69, 442, 342, 0x001);  // canvas border

   frame_p->drawCircle(60, 110, 30, 0x001);  // color palette border

   frame_p->fillCircle(40, 170, 5, 0x001);
   frame_p->fillCircle(75, 170, 8, 0x001);
   frame_p->fillCircle(40, 210, 11, 0x001);
   frame_p->fillCircle(75, 210, 16, 0x001);

   frame_p->drawRect(38, 250, 15, 15, 0x001);   // black
   frame_p->fillRect(39, 251, 13, 13, 0x001);

   frame_p->drawRect(68, 250, 15, 15, 0x001);   // red
   frame_p->fillRect(69, 251, 13, 13, color0);


   frame_p->drawRect(38, 270, 15, 15, 0x001);   // orange
   frame_p->fillRect(39, 271, 13, 13, color1);

   frame_p->drawRect(68, 270, 15, 15, 0x001);   // yellow
   frame_p->fillRect(69, 271, 13, 13, color2);


   frame_p->drawRect(38, 290, 15, 15, 0x001);   // green
   frame_p->fillRect(39, 291, 13, 13, color3);

   frame_p->drawRect(68, 290, 15, 15, 0x001);   // blue
   frame_p->fillRect(69, 291, 13, 13, color4);


   frame_p->drawRect(38, 310, 15, 15, 0x001);   // purple
   frame_p->fillRect(39, 311, 13, 13, color5);

   frame_p->drawRect(68, 310, 15, 15, 0x001);   // pink
   frame_p->fillRect(69, 311, 13, 13, color6);


   frame_p->drawRect(38, 330, 15, 15, 0x001);   // brown
   frame_p->fillRect(39, 331, 13, 13, color7);

   frame_p->drawRect(68, 330, 15, 15, 0x001);   // white
   frame_p->fillRect(69, 331, 13, 13, 0xfff);


   frame_p->drawRect(37, 382, 45, 20, 0x001);

}

void color_palette(FrameCore *frame_p, int color) {
   frame_p->fillCircle(60, 110, 29, color);
}

uint16_t map_brush(float mag, float in_min, float in_max, int out_min, int out_max) {

   return (uint16_t)((mag - in_min) * (out_max - out_min) / (in_max - in_min) + out_min);
}

void draw_brush(FrameCore *frame_p, int x, int y, int color, int size) {   // function for drawing
   frame_p->fillCircle(x, y, size, color);   // call fillCircle function with passed parameters
}

float tapper(SpiCore *spi_p) {   // function to get accelerometer reading from board

   const uint8_t RD_CMD = 0x0b;
   const uint8_t PART_ID_REG = 0x02;
   const uint8_t DATA_REG = 0x08;
   const float raw_max = 127.0 / 2.0;  //128 max 8-bit reading for +/-2g

   int8_t xraw, yraw, zraw;
   float x, y, z;
   int id;
   float mag;

   spi_p->set_freq(400000);
   spi_p->set_mode(0, 0);
   // check part id
   spi_p->assert_ss(0);    // activate
   spi_p->transfer(RD_CMD);  // for read operation
   spi_p->transfer(PART_ID_REG);  // part id address
   id = (int) spi_p->transfer(0x00);
   spi_p->deassert_ss(0);
   // uart.disp("read ADXL362 id (should be 0xf2): ");
   // uart.disp(id, 16);
   // uart.disp("\n\r");
   // read 8-bit x/y/z g values once
   spi_p->assert_ss(0);    // activate
   spi_p->transfer(RD_CMD);  // for read operation
   spi_p->transfer(DATA_REG);  //
   xraw = spi_p->transfer(0x00);
   yraw = spi_p->transfer(0x00);
   zraw = spi_p->transfer(0x00);
   spi_p->deassert_ss(0);
   x = (float) xraw / raw_max;
   if(x < 0)
      x = -x;
   y = (float) yraw / raw_max;
   if(y < 0)
      y = -y;
   z = (float) zraw / raw_max;
   if(z < 0)
      z = -z;

   mag = x + y + z;

   return mag;
   
}

void select(FrameCore *frame_p, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
   frame_p->drawRect(x, y, w, h, color);
}



// external core instantiation
GpoCore led(get_slot_addr(BRIDGE_BASE, S2_LED));
GpiCore sw(get_slot_addr(BRIDGE_BASE, S3_SW));
FrameCore frame(FRAME_BASE);
GpvCore bar(get_sprite_addr(BRIDGE_BASE, V7_BAR));
GpvCore gray(get_sprite_addr(BRIDGE_BASE, V6_GRAY));
SpriteCore ghost(get_sprite_addr(BRIDGE_BASE, V3_GHOST), 1024);
SpriteCore mouse(get_sprite_addr(BRIDGE_BASE, V1_MOUSE), 1024);
OsdCore osd(get_sprite_addr(BRIDGE_BASE, V2_OSD));
SsegCore sseg(get_slot_addr(BRIDGE_BASE, S8_SSEG));

Ps2Core ps2(get_slot_addr(BRIDGE_BASE, S11_PS2));
XadcCore adc(get_slot_addr(BRIDGE_BASE, S5_XDAC));
PwmCore pwm(get_slot_addr(BRIDGE_BASE, S6_PWM));
SpiCore spi(get_slot_addr(BRIDGE_BASE, S9_SPI));

int main() {

   srand(NULL);
   int id = ps2_init(&ps2);   // grab mouse ID
   int x = 320;   // x coordinate position of the mouse (initialized to center of screen)
   int y = 240;   // y coordinate position ofthe mouse (initialized to center of screen)
   int btn_left, btn_right, xcord, ycord = 0; // take mouse info and use to move sprite, etc
   double r, g, b = 0.0;   // RGB values from the spectrum function
   uint16_t rint, gint, bint; // RGB values converted to uint from double
   uint16_t color = 0;  // RGB values concatenated together R-G-B: 4-4-4
   int brush_size = 5;  // brush size when drawing circles (initialized to radius of 5)

   float mag_old = 0;   // previous magnitude of accelerometerreading (initialized to 0)
   float spike;   // spike value of accelerometer (the difference between the new and old magnitudes)

   double colorpot_old = 0.0;
   double brushpot_old = 0.0;
   bool colorpotflag = false;
   bool brushpotflag = false;

   int color0 = 0xf00;
   int temporary = rand() % 4094;
   int color1 = rand() % 4094;
   int color2 = rand() % 4094;
   int color3 = rand() % 4094;
   int color4 = rand() % 4094;
   int color5 = rand() % 4094;
   int color6 = rand() % 4094;
   int color7 = rand() % 4094;

   /* Turn off all seven segments */
   sseg.write_1ptn(0b1111111,0);
   sseg.write_1ptn(0b1111111,1);
   sseg.write_1ptn(0b1111111,2);
   sseg.write_1ptn(0b1111111,3);
   sseg.write_1ptn(0b1111111,4);
   sseg.write_1ptn(0b1111111,5);
   sseg.write_1ptn(0b1111111,6);
   sseg.set_dp(0);

   /* Bypass all cores */
   frame.bypass(1);
   bar.bypass(1);
   gray.bypass(1);
   ghost.bypass(1);
   osd.bypass(1);
   mouse.bypass(1);

   initialize_canvas(&frame, color0, color1, color2, color3, color4, color5, color6, color7); // initialize the canvas by setting screen to white
   welcome_msg(&osd);   // call the welcome message
   trademark(&osd); // display trademark

   while (1) {

      int ret; // for errors

      double colorpot_new = pot_value_color(&adc); // grab the color potentiometer adc value
      double colorpot_diff = colorpot_new - colorpot_old;   // calc the difference between the previous and current adc value
      if (colorpot_diff < 0 ) 
         colorpot_diff = colorpot_diff * -1; // keep the difference positive (absolute/magnitude value)

      if(colorpot_diff > 0.008)
         colorpotflag = true;    // if above threshold difference of 0.008, then set flag to use potentiometer color for brush


      double brushpot_new = pot_value_brush(&adc); // grab the brush potentiometer adc value
      double brushpot_diff = brushpot_new - brushpot_old;   // calc the difference betwen the previous and current adc value
      if (brushpot_diff < 0 ) 
         brushpot_diff = brushpot_diff * -1; // keep the difference positive (absolute/magnitude value)

      if(brushpot_diff > 0.008)
         brushpotflag = true;    // if above threshold difference of 0.008, then set flag to use potentiometer brush size


      if(colorpot_new != colorpot_old) { // if there is a change in potentiometer value, update 
         colorpot_old = colorpot_new;
         if(colorpotflag){ // if change is above the threshold
            spectrum(&pwm, colorpot_new, &r, &g, &b);   // call the RGB LED spectrum function
            // map double rgb values from 0-15 uint
            rint = map_rgb(r, 0.03, .999, 0, 0xf);
            gint = map_rgb(g, 0.03, .999, 0, 0xf);
            bint = map_rgb(b, 0.03, .999, 0, 0xf);

            
            // concatenate the individual rgb uints
            uint16_t temp = rint << 8;
            uint16_t temp2 = gint << 4;
            uint16_t temp3 = bint;
            color = (temp) | (temp2) | (temp3);

            // uart.disp("COLOR pot\n\r");
         }
      }

      if(brushpot_new != brushpot_old) {  // if there is a change in potentiometer value, update
         brushpot_old = brushpot_new;
         if(brushpotflag) {   // if change is above the threshold
            brush_size = map_brush(brushpot_new, 0.03, 0.999, 1, 20);  // map brush sizes from 1-15 radius
            select(&frame, 28, 159, 25, 25, 0xA8B);   // clear all borders around brushes bc we are using pot now
            select(&frame, 63, 159, 25, 25, 0xA8B);
            select(&frame, 28, 199, 25, 25, 0xA8B);
            select(&frame, 58, 194, 35, 35, 0xA8B);
            // uart.disp("BRUSH pot\n\r");
         }
         
      }

      
      color_palette(&frame, color); // update the palette color on screen
      

      // take the mouse info and RGB info and use to paint, etc
      ret = canvas_mouse(&ps2, &sseg, id, &btn_left, &btn_right, &xcord, &ycord);
      int tmpcount = 1; // temp counter make sure welcome msg off runs only once
      if(ret) {   // only if mouse data is valid (if you move/click the mouse)

         if(tmpcount) {
            welcome_msg_off(&osd);  // turn off welcome message
            tmpcount = 0;
         }
         // uart.disp("[");
         // uart.disp(btn_left);
         // uart.disp(", ");
         // uart.disp(btn_right);
         // uart.disp(", ");
         // uart.disp(xcord);
         // uart.disp(", ");
         // uart.disp(ycord);
         // uart.disp("] \r\n");

         move_brush(&mouse, xcord, ycord, &x, &y); // move the brush to the new mouse position using the mouse data

         if((x > 100 + brush_size && x < 540 - brush_size) && (y > 70 + brush_size && y < 410 - brush_size)) { // drawing boundaries for the canvas
            if(btn_left) { // if you are left clicking
               draw_brush(&frame, x, y, color, brush_size); // draw a circle where the cursor is
               // uart.disp("left click\n\r");
            }
            if(btn_right) {   // if you are right clicking
               draw_brush(&frame, x, y, 0xfff, brush_size); // draw a WHITE circle where the cursor is (to simulate erasing)
               // uart.disp("right click\n\r");
            }
         }
         else {   // if not within canvas drawing boundaries

            if(btn_left) { // check if left clicking outside of canvas
               // CLICK ON BRUSH SIZES
               if((x > 28 && x < 52) && (y > 158 && y < 182)) {   // if left click on brush size 5 circle
                  brush_size = 5;   // set brush size
                  brushpotflag = false;   // unset flag so user is not using potentiometer value for brush size
                  select(&frame, 28, 159, 25, 25, 0x001);   // draw border around brush 5
                  select(&frame, 63, 159, 25, 25, 0xA8B);   // clear borders around previous brushes
                  select(&frame, 28, 199, 25, 25, 0xA8B);
                  select(&frame, 58, 194, 35, 35, 0xA8B);
                  uart.disp("CLICK BRUSH 5\n\r");
               }

               if((x > 63 && x < 87) && (y > 158 && y < 182)) {   // if left click on brush size 8 circle
                  brush_size = 8;   // set brush size
                  brushpotflag = false;   // unset flag so user is not using potentiometer value for brush size
                  select(&frame, 63, 159, 25, 25, 0x001);   // draw border around brush 8
                  select(&frame, 28, 159, 25, 25, 0xA8B);   // clear borders around previous brushes
                  select(&frame, 28, 199, 25, 25, 0xA8B);
                  select(&frame, 58, 194, 35, 35, 0xA8B);
                  uart.disp("CLICK BRUSH 8\n\r");
               }

               if((x > 28 && x < 52) && (y > 198 && y < 222)) {   // if left click on brush size 11 circle
                  brush_size = 11;  // set brush size
                  brushpotflag = false;   // unset flag so user is not using potentiometer value for brush size
                  select(&frame, 28, 199, 25, 25, 0x001);   // draw border around brush 11
                  select(&frame, 28, 159, 25, 25, 0xA8B);   // clear borders around previous brushes
                  select(&frame, 63, 159, 25, 25, 0xA8B);
                  select(&frame, 58, 194, 35, 35, 0xA8B);
                  uart.disp("CLICK BRUSH 11\n\r");
               }

               if((x > 58 && x < 92) && (y > 193 && y < 227)) {   // if left click on brush size 16 circle
                  brush_size = 16;  // set brush size
                  brushpotflag = false;   // unset flag so user is not using potentiometer value for brush size
                  select(&frame, 58, 194, 35, 35, 0x001);   // draw border around brush 16
                  select(&frame, 28, 159, 25, 25, 0xA8B);   // clear borders around previous brushes
                  select(&frame, 63, 159, 25, 25, 0xA8B);
                  select(&frame, 28, 199, 25, 25, 0xA8B);
                  uart.disp("CLICK BRUSH 16\n\r");
               }

               // CLICK ON PAINT COLORS
               if((x > 37 && x < 53) && (y > 249 && y < 266)) {   // if left click on black color
                  color = 0x001; // set brush color
                  colorpotflag = false;   // unset flag so user is not using potentiometer value for brush color
                  uart.disp("CLICK COLOR Black\n\r");
               }

               if((x > 67 && x < 84) && (y > 249 && y < 266)) {   // if left click on red color
                  color = color0; // set brush color
                  colorpotflag = false;   // unset flag so user is not using potentiometer value for brush color
                  uart.disp("CLICK COLOR Red\n\r");
               }

               if((x > 37 && x < 53) && (y > 269 && y < 286)) {   // if left click on orange color
                  color = color1; // set brush color
                  colorpotflag = false;   // unset flag so user is not using potentiometer value for brush color
                  uart.disp("CLICK COLOR Orange\n\r");
               }

               if((x > 67 && x < 84) && (y > 269 && y < 286)) {   // if left click on yellow color
                  color = color2; // set brush color
                  colorpotflag = false;   // unset flag so user is not using potentiometer value for brush color
                  uart.disp("CLICK COLOR Yellow\n\r");
               }

               if((x > 37 && x < 53) && (y > 289 && y < 306)) {   // if left click on green color
                  color = color3; // set brush color
                  colorpotflag = false;   // unset flag so user is not using potentiometer value for brush color
                  uart.disp("CLICK COLOR Green\n\r");
               }

               if((x > 67 && x < 84) && (y > 289 && y < 306)) {   // if left click on blue color
                  color = color4; // set brush color
                  colorpotflag = false;   // unset flag so user is not using potentiometer value for brush color
                  uart.disp("CLICK COLOR Blue\n\r");
               }

               if((x > 37 && x < 53) && (y > 309 && y < 326)) {   // if left click on purple color
                  color = color5; // set brush color
                  colorpotflag = false;   // unset flag so user is not using potentiometer value for brush color
                  uart.disp("CLICK COLOR Purple\n\r");
               }

               if((x > 67 && x < 84) && (y > 309 && y < 326)) {   // if left click on pink color
                  color = color6; // set brush color
                  colorpotflag = false;   // unset flag so user is not using potentiometer value for brush color
                  uart.disp("CLICK COLOR Pink\n\r");
               }

               if((x > 37 && x < 53) && (y > 329 && y < 346)) {   // if left click on brown color
                  color = color7; // set brush color
                  colorpotflag = false;   // unset flag so user is not using potentiometer value for brush color
                  uart.disp("CLICK COLOR Brown\n\r");
               }

               if((x > 67 && x < 84) && (y > 329 && y < 346)) {   // if left click on white color
                  color = 0xfff; // set brush color
                  colorpotflag = false;   // unset flag so user is not using potentiometer value for brush color
                  uart.disp("CLICK COLOR White\n\r");
               }

               // LEFT CLICK ON CLEAR 
               if((x > 36 && x < 82) && (y > 381 && y < 403)) {   // if leftclick on clear button
                  initialize_canvas(&frame, color0, color1, color2, color3, color4, color5, color6, color7); // clear canvas
               }


            }

         }
         
      }
           

      float mag_new = tapper(&spi); // grab the new magnitude from the accelerometer
      spike = mag_new - mag_old; // calc the difference between the magnitudes (spike)


      if(mag_new != mag_old)  // if there is a spike, update the magnitudes
         mag_old = mag_new;

      if(spike > 1.0) { // if spike exceeds a certain threshhold
         initialize_canvas(&frame, color0, color1, color2, color3, color4, color5, color6, color7); // clear the canvas
         // uart.disp("canvas cleared\n\n\r");
      }
         

   } //while


} //main
