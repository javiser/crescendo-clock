# Crescendo Clock - an ESP32 based alarm clock to wake up gently
A self-made clock with a gentle crescendo functionality.

## Highlights
- Written in C++ with ESP-IDF framework (no Arduino!) using Platformio + VSCode
- Small but not just an empty "hello world" example
- Available custom bootloader and partition tables for this board
- Preconfigured runner, just call `cargo run`

## Hardware
Work in progress
### Electrical components
- ESP32C3
- Rotary encoder
- 2.4 inch display with ILI9341 driver
- DFRobot's DFPlayer Mini
- Light sensor
- Resistance, capacitors, wires...

### Prototype build
(picture)
### Further ideas
- Board
- Case
etc

## Software
This part of the documentation is still work in progress, as the software itself. Just as a teaser: in order to compile and flash the SW I used:
- Linux development environment and USB connection to the board
- Visual Studio Code with platformio extension
- Espressif platform v4.4 installed via platformio

## Credits and acknowledgment
For this project I have used the inspiration and code from many other projects and sources: 
- The project template has been created using the platformio extension for Visual Code
- I looked up and partly copied some code from the official esp-idf examples contained in https://github.com/espressif/esp-idf/tree/master/examples (mainly those related to SNTP, WPS and Wifi functions)
- For the rotary encoder I used some code from https://github.com/LennartHennigs/ESPRotary as well as from https://github.com/craftmetrics/esp32-button. The code from https://github.com/DavidAntliff/esp32-rotary-encoder was also a good source of inspiration
- For the finite state machine implementation I borrowed the ideas from https://www.aleksandrhovhannisyan.com/blog/finite-state-machine-fsm-tutorial-implementing-an-fsm-in-c/ as well as from https://stackoverflow.com/questions/14676709/c-code-for-state-machine
- For the DFPlayer module used this datasheet to look up the specifications https://cdn.shopify.com/s/files/1/1509/1638/files/MP3_Player_Modul_Datenblatt.pdf?10537896017176417241. I also found some undocumented functions in the library implementation in https://github.com/DFRobot/DFRobotDFPlayerMini

## License
Copyright (c) 2022 javiser
`crescendo-clock` is distributed under the terms of the MIT License.

See the [LICENSE](LICENSE) for license details.

Credits / inspirations:

