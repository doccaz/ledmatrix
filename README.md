# ledmatrix
A LED matrix scroller with WiFi using WeMOS D1.

# the hardware

This uses eight MAX7219 8x8 LED matrix displays in series.

I used two 4-display from Banggood: https://www.banggood.com/en/MAX7219-Dot-Matrix-Module-4-in-1-Display-For-Arduino-p-1072083.html, and just connected them to each other.

For the microcontroller, I used a Wemos D1 Mini Pro. This microcontroller is ESP8266-based, and has both bluetooth and WiFi, and has 16MB of flash onboard. There is an Arduino IDE plugin available from www.wemos.cc. I bought mine from here: https://www.banggood.com/Wemos-D1-Mini-V3_0_0-WIFI-Internet-Of-Things-Development-Board-Based-ESP8266-4MB-p-1264245.html

I used a "1-button shield" on top of the Wemos for resetting/displaying configuration. It's wired to D3.
This shield can be bought here: https://www.banggood.com/WeMos-1-Button-Shield-For-WeMos-D1-Mini-Button-p-1160501.html

# features

So far I added the following features:
 - controllable speed
 - controllable brightness
 - customizable SSID and password for the wifi
 - all parameters are saved on the internal flash
 - persistent message (saved in flash, survives power cycling)

# software requirements

You'll need: 

- Arduino IDE with WeMOS support added (from wemos.cc)
- Adafruit GFX library
- SPIFFS library

# about SPIFSS

SPIFFS is a (very) simple filesystem supported on the WeMOS D1. Some batches appear to not work out of the box when we need to format the filesystem.

A change needs to be done in Arduino15/packages/esp8266/hardware/esp8266/2.4.0/cores/esp8266/Esp.cpp in order for it to work with Wemos.

Add the following line:

  static const int FLASH_INT_MASK = ((B10 << 8) | B00111010);

right before this line:

  bool EspClass::flashEraseSector(uint32_t sector) {



# putting it all together

Connections:
  D5 -> CLK (clock)
  D7 -> DIN (data IN)
  D4 -> CS (chip select)
  GND -> GND
  5v -> VCC

The 1-button shield is wired to D3.

The SPIFFS filesystem must first be initialized with SPIFFS.format(). This can take a few minutes. This allows the configuration file to be read/written to. Use the provided sketch (spifftest.ino).

Then, upload the matrixdisplay.ino sketch to the WeMOS D1, and you're done!

# using it

 - Connect to the SSID with the password shown.
 - Access http://<IP> on a browser and input a message.

To change configuration:
 - Connect to the SSID with the password shown.
 - Access http://<IP>/setup and change the config information.

To reset the configuration:
 - Power up (or press the reset button on the side)
 - while "starting" is being displayed, press and hold the shield button.
 - "reset configuration" will be shown
 - connect to the new SSID with the information shown
 
It works just fine from a regular 500mA USB port. A 10.000mA USB powerbank was able to power it for at least 12 hours.


# to-do
 - add HTML template support, so we can change themes on the web UI
 - add accented characters to the standard Adafruit font
 - add scroll effects
 - add RTC support
 

 Have fun!
