# FPGA-Digital-Canvas
Pixel Poet is an FPGA digital canvas created using a bare metals SoC on the Nexys A7 FPGA Board (similar to Paint by Microsoft).

[HDL Files]()

[Driver Files]()

[Main C File]()

## The Project

PixelPoet is the name of the digital canvas created in this project. PixelPoet's functions include a PS2 protocol mouse to control a paintbrush sprite around the VGA screen. There are also two XADC potentiometers
to control the color of the brush and the size of the brush. An SPI protocol accelerometer can clear the canvas upon a shake of the Nexys A7 board. Everything else on the VGA display is generated using
the frame buffer hardware core along with its appropriate driver.

- [Main C File]()
  - All functions are coded inside of this main file.
- [vgaCore]()
  - Additional graphical shapes from ADAFRUIT added to this file.

