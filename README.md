# FPGA-Digital-Canvas
Pixel Poet is an FPGA digital canvas created using a bare metals SoC on the Nexys A7 FPGA Board (similar to Paint by Microsoft).

[HDL Files](https://github.com/ishwo0/FPGA-Digital-Canvas/tree/main/HDL%20Files)

[Driver Files](https://github.com/ishwo0/FPGA-Digital-Canvas/tree/main/Driver%20Files)

[Main C File](https://github.com/ishwo0/FPGA-Digital-Canvas/blob/main/Main%20File/main.cpp)

## The Project

PixelPoet is the name of the digital canvas created in this project. PixelPoet's functions include a PS2 protocol mouse to control a paintbrush sprite around the VGA screen. There are also two XADC potentiometers
to control the color of the brush and the size of the brush. An SPI protocol accelerometer can clear the canvas upon a shake of the Nexys A7 board. Everything else on the VGA display is generated using
the frame buffer hardware core along with its appropriate driver.

- [Main C File](https://github.com/ishwo0/FPGA-Digital-Canvas/blob/main/Main%20File/main.cpp)
  - All functions are coded inside of this main file.
- [vgaCore](https://github.com/ishwo0/FPGA-Digital-Canvas/blob/main/Driver%20Files/vga_core.h)
  - Additional graphical shapes from ADAFRUIT added to this file.

