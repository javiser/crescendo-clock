# Crescendo Clock - an ESP32 based alarm clock to wake up gently
A self-made clock with a gentle crescendo functionality.

## Highlights
- Gentle alarm-clock with a crescendo function to wake up slowly and peacefully
- Simplified user interaction with a rotary encoder with a button. No touch display, no need for 10 different buttons
- Custom 3D printed case
- Integration with home assistant via MQTT messages (i.e. to create a wake up light effect)
- Written in C++ with ESP-IDF framework (no Arduino!) using Platformio + VSCode

## Hardware
### Electrical components
This list shows the components I chose but many of them can be surely replaced with other equivalent components without further changes, while others might require small adaptions (i.e. display driver configuration, pinout, etc.). See [below](#possible-modifications-and-variants) for some ideas.
- [ESP-C3-32S-Kit ESP32 WiFi+Bluetooth Development Board](https://www.waveshare.com/esp-c3-32s-kit.htm) with ESP32C3 chip from Ai-Thinker which I have "tuned" to get rid of the built-in LEDs. 
- [Rotary encoder with board](https://www.ebay.de/itm/233671849958)
- [2.4 inch display with ILI9341 driver](https://www.waveshare.com/2.4inch-lcd-module.htm)
- [DFRobot's DFPlayer Mini](https://www.dfrobot.com/product-1121.html)
- [Light sensor](https://www.adafruit.com/product/2748)
- [Mini-speaker](https://www.ebay.de/itm/313914312809)
- Miscellaneous components, like resistors, capacitors, wires...

### Breadboard prototype
In this prototype I used a [Sparkfun's RGB-Rotary encoder](https://www.sparkfun.com/products/15141), which I then discarded in favor of a simpler model, since ni the end I decided not to use the encoder LEDs.

![](hardware/pictures/breadbord_prototype.png)
### Further ideas and open tasks
[ ] Choose an adequate speaker
[ ] Create a custom board to connect everything together

### Possible modifications and variants
The above components are just what I could get my hands on or what made sense to me. Here some considerations:
- It should be possible to use any other ESP32 board, specially ESP32C3 like the official [ESP32-C3-DevKitM-1](https://docs.espressif.com/projects/esp-idf/en/latest/esp32c3/hw-reference/esp32c3/user-guide-devkitm-1.html) from Espressif. But the board design and/or pin assignments defined in [clock_common.hpp](src/clock_common.hpp) may need a corresponding adjustment.
- The light functions of the rotary encoder are not required for the clock to work. A normal rotary encoder should work. Also the [RG-rotary encoder](https://www.sparkfun.com/products/15140) from Sparkfun should work exactly the same as the one I used.
- Other LCD displays could work as well, specially if the same ILI9341 driver is used, but maybe other settings are required.

## Software
This part of the documentation is still work in progress, as the software itself. Just as a teaser: in order to compile and flash the SW I used:
- Linux development environment and USB connection to the board
- Visual Studio Code with platformio extension
- Espressif platform v6.1 installed via platformio. Platform updates may cause the code to break, this happened during development several times. If you want to avoid this, set the line `platform = espressif32@=6.1.0` in the file [platformio.ini](platformio.ini)

## Credits and acknowledgment
For this project I have used the inspiration and code from many other projects and sources: 
- The project template has been created using the platformio extension for Visual Code
- I looked up and partly copied some code from the official esp-idf examples contained in https://github.com/espressif/esp-idf/tree/master/examples (mainly those related to SNTP, WPS and Wifi functions)
- For the rotary encoder I used some code from https://github.com/LennartHennigs/ESPRotary as well as from https://github.com/craftmetrics/esp32-button. The code from https://github.com/DavidAntliff/esp32-rotary-encoder was also a good source of inspiration
- For the finite state machine implementation I borrowed the ideas from https://www.aleksandrhovhannisyan.com/blog/finite-state-machine-fsm-tutorial-implementing-an-fsm-in-c/ as well as from https://stackoverflow.com/questions/14676709/c-code-for-state-machine
- For the DFPlayer module used this datasheet to look up the specifications https://cdn.shopify.com/s/files/1/1509/1638/files/MP3_Player_Modul_Datenblatt.pdf?10537896017176417241. I also found some undocumented functions in the library implementation in https://github.com/DFRobot/DFRobotDFPlayerMini

## License
Copyright (c) 2023 javiser
`crescendo-clock` is distributed under the terms of the MIT License.

See the [LICENSE](LICENSE) for license details.

