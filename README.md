AfroGovernor
============

A governor for the AfroESC, see: http://hobbyking.com/hobbyking/store/__869__710__Multi_Rotors_Parts-Afro_ESC_Series.html.

The governor itself runs on a Leonardo micro pro (clone) running LUFA with a slighly hacked DFU bootloader
rather than the arduino bootloader.

The ESC runs the fork of SimonKs ESC firmware that was made by Andrew Zaborowski (aka. balrog-kun)
https://github.com/balrog-kun/tgy to allow proper I2C support.

The governor talks to the ESC via I2C, this allows the governor to read back the actual speed of the motor
without any additional hardware.


Hardware
========

The hardware is just a couple of 10k resistors from the two I2C lines to vcc and 4 wires:

Governor  <->  ESC

* GND     <->  GND (1)
* VCC     <->  VCC (2)
* SDA (2) <->  SDA (5)
* SCL (3) <->  SCL (6)

The ESC has the connections labeled on the through-hole pads, the numbers refer to the hole-position, counting
from the power-in side of the board.


Firmware: DFU bootloader with timeout
=====================================

I wanted to use the DFU bootloader, rather than ISP for programming the Leonardo, but I also want the
governor to start working on its own and the android guys tied the bootloader pin to the rail that means
that the bootloader always runs, so I had to add a timeout to the DFU bootloader, this is the purpose of
the bootloader-dfu-with-timeout directory which was copied from the lufa-LUFA-140302/Bootloaders/DFU dir.


Firmware: Governor
==================

The governor itself lives in the governor directory and it's basically a very modified version of the
LUFA example called VirtualSerial


Firmware: lufa-LUFA-140302 & balrog-kun's tgy fork
==================================================

I've included a copy of LUFA and balrog-kun's code, because that was easier than writing a proper build script,
but I did not modify those bits in any way.


Flashing a new ATMega8 for the AfroESC
======================================

During testing I managed to destroy the MCU on the ESC, so after replacing the ATmega8 I had to figure out
the ISP connections for the ESC to flash the new MCU, it was quite easy as the traces are helpfully visible
for most of the pads, but to save others the trouble I've included an annotated scan of the board with the ISP
pads labeled, see afro-esc-20a.jpeg

Once the programmer is hooked up (I made a quick board with a 6-pin header and 6 bits of wirewrap wire) two
things need to be programmed:

* The fuses:    avrdude -c usbtiny -p m8 -U lfuse:w:0x8e:m -U hfuse:w:0xd2:m
* The firmware: avrdude -c usbtiny -p m8 -U flash:w:tgy/afro_nfet.hex

I could not find the fuse settings anywhere, so I took a wild guess and it turned out ok.



But why, oh why did I do such a thing?
======================================

I have a no-name icecream maker which was very cheap, it as a nice compressor and freezes quickly enough, but
the motor is a weedy 11W 220V thing that runs much too fast and has too little torque to churn icecream until
it's finished.

To remedy the motor problem I sourced a 320kv (read:slow) turnigy multistar: 
http://hobbyking.com/hobbyking/store/__45281__4114_320KV_Turnigy_Multistar_Multi_Rotor_Motor_With_3_5mm_Bullet_Connector.html

The new motor has much more torque, but it needs active control to keep running a constant speed, unfortunately
there's no off-the shelf ESC with a governor that can run as slow as I want this motor to go, so I had to roll
my own solution.