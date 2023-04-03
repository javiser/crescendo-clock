# Crescendo Clock - an ESP32 based alarm clock to wake up gently

<p align="center">
  <a href="#highlights">Highlights</a> •
  <a href="#how-to-use">How to use</a> •
  <a href="#main-components">Main components</a> •
  <a href="#compiling-and-customizing-the-software">Compiling and customizing SW</a> •
  <a href="#credits-and-acknowledgment">Credits</a><br>
</p>

**A self-made clock with a gentle crescendo wake up alarm**

## Highlights
- Gentle alarm-clock with a crescendo function to wake up slowly and peacefully
- Simplified user interaction with a rotary encoder with a button. No touch display, no need for 10 different buttons
- Convenient WiFi connection via WPS. Credentials are then stored in NVS flash of the board
- Custom 3D printed case
- Integration with home assistant via MQTT messages (i.e. to create a wake up light effect)
- Light sensor to turn off display at adjust the brightness dynamically
- Written in C++ with ESP-IDF framework (no Arduino!) using Platformio + VSCode

## How to use
### Interaction elements
The user interaction with the crescendo clock takes place exclusively with the rotary encoder. This supports four possible actions:
- Left rotation
- Right rotation
- Short press / click
- Long press

Double press / click action is not supported.

### First step: connecting to WiFi
The crescendo clock sets the time automatically when it is connected to a WiFi. Manual time setting is not supported. The first time you power the clock or when the configured WiFi is not available, you will see a red wifi symbol on the right side of the screen:


Rotate the encoder just one click (left or right) and you will see a message "PRESS WPS". Press the WPS button of your router (or select the corresponding option of your specific router model) and wait a little bit until the WiFi connection has been established automatically. WiFi credentials are stored in NVS of ESP32 so that you will not need to repeat this step in the future. Only one credentials set can be stored: if you configure your clock for a different WiFi using the same procedure, the original WiFi credentials will no longer be present in the NVS.

### Using the crescendo clock 
This overview describes the basic usage of the crescendo clock:
| State           |              Rotation              |     Short press     |         Long press         |
|-----------------|:----------------------------------:|:-------------------:|:--------------------------:|
| Time            | Increase brightness / activate WPS | Set alarm on/off    | Enter alarm time           |
| Set alarm time  | Change hour / minute of alarm      | Next step           | Cancel alarm setting       |
| Alarm triggered | No effect                          | Snooze              | No effect                  |
| Alarm snooze    | Snooze cancelling sequence         | Increase brightness | Snooze cancelling sequence |

Additional comments:
- If the alarm is active for triggering the alarm time is shown, otherwise it is hidden
- If the alarm is active and the remaining time until alarm triggering is less than 9 hours, this remaining "bed time" will be displayed as well
- After snoozing a triggered alarm, the alarm will be triggered again after 5 minutes if the snooze has not been cancelled
- The snooze process will continue indefinitely until it hasn't been cancelled
- The snooze cancelling sequence is like this: rotate the encoder in one direction, then long press the encoder and finally rotate the encoder in the opposite direction, waiting no longer than 3 seconds between steps. You will see one bar above the remaining time until new alarm trigger when the first step of the sequence has been successfully performed and a second bar after the second step. When the final step is performed, the remaining "snooze time" will just disappear and the alarm time will disappear as well (see the video at the beginning of this page)
- After the snooze cancelling sequence the alarm is deactivated until you activate it again manually. You need to set the alarm every single day! Remember, I designed the clock following my ideal concept of a clock and this is the way I like it.

### MQTT integration
The crescendo clock does not subscribe to any MQTT message. Instead, it sends two different messages:
- One message with the topic `wecker/wake_up` when the alarm is triggered. This will be send every time the alarm is triggered, also after the snooze phase.
- Another message with the topic `wecker/alarm_off` when the snooze cancelling sequence is complete and the alarm has been completely deactivated.

## Main components
These are the main electrical components used in this project:
- [Seeed Studio XIAO ESP32C3 board](https://wiki.seeedstudio.com/XIAO_ESP32C3_Getting_Started/)
- [Rotary encoder with board](https://www.ebay.de/itm/173657244984)
- [2.4 inch display with ILI9341 driver](https://www.waveshare.com/2.4inch-lcd-module.htm)
- [DFRobot's DFPlayer Mini](https://www.dfrobot.com/product-1121.html)
- [Light sensor](https://www.adafruit.com/product/2748)
- [Mini-speaker](https://www.ebay.de/itm/313914312809)

See the [detailed description](hardware/README.md) in the hardware folder for more information about schematics, PCB, 3D printed case, detailed assembly instructions and other design considerations.

## Compiling and customizing the software
Basic knowledge about platformio, ESP-IDF development and ESP32 boards is required. This project has been tested only under a Linux environment. In order to compile this project you will need:
- Your populated PCB board with the ESP32C3 board connected to the PC via USB
- Visual Studio Code with platformio extension installed and Espressif platform 6.1 installed. Platform updates may cause the code to break, this happened during development several times. If you want to avoid this, set the line `platform = espressif32@=6.1.0` in the file [platformio.ini](platformio.ini)

Just build and upload the code! If you upload to code to the board with no hardware connected to it (display, encoder, etc.) you should at least be able to see some basic debug messages via Serial Monitor related to the failed WiFi connection.

### Waking melodies and other settings
In this repository there is not any audio file for the waking melody. In the [DFPlyer mini wiki](https://wiki.dfrobot.com/DFPlayer_Mini_SKU_DFR0299) you will find instructions on how to create your own files (see chapter ["Copy your mp3 into you micro SD card"](https://wiki.dfrobot.com/DFPlayer_Mini_SKU_DFR0299#target_6)). I have a folder called `mp3` in the SD-card and inside it a single file called `0001.mp3`. You can add further files and change the value of `melody_nr` in the `settings` struct in [src/clock_machine.hpp](src/clock_machine.hpp) and add the corresponding mp3 file. In case you want a confirmation sound when you activate the alarm, then set the settings variable `alarm_set_confirmation_sound` to true and make sure a (short) `0101.mp3` file exists. This does not work really well, as the DFPlayer doesn't seem to like short audio files, and I will most likely remove this feature in a near future.

Furthermore, you can change also the snooze time (default = 5 minutes) and the "crescendo speed" in the same `settings` structure. These are fixed values and cannot be changed after compilation.

## Credits and acknowledgment
For this project I have used the inspiration and code from many other projects and sources: 
- I looked up and partly copied some code from the official esp-idf examples contained in https://github.com/espressif/esp-idf/tree/master/examples (mainly those related to SNTP, WPS, MQTT and Wifi functions)
- For the rotary encoder I used some code from https://github.com/LennartHennigs/ESPRotary as well as from https://github.com/craftmetrics/esp32-button. The code from https://github.com/DavidAntliff/esp32-rotary-encoder was also a good source of inspiration
- For the finite state machine implementation I borrowed the ideas from https://www.aleksandrhovhannisyan.com/blog/finite-state-machine-fsm-tutorial-implementing-an-fsm-in-c/ as well as from https://stackoverflow.com/questions/14676709/c-code-for-state-machine
- For the DFPlayer module used this datasheet to look up the specifications https://cdn.shopify.com/s/files/1/1509/1638/files/MP3_Player_Modul_Datenblatt.pdf?10537896017176417241. I also found some undocumented functions in the library implementation in https://github.com/DFRobot/DFRobotDFPlayerMini
- [Adafruit](https://www.adafruit.com/) for the many resources I used for the creation of custom fonts and symbols (see a [more detailed description](fonts/README.md) in the resources folder) as well as for the great inspiration from their [PyPortal Alarm Clock](https://learn.adafruit.com/pyportal-alarm-clock), out of which I created a very first concept of this kind of crescendo clock. 

## License
Copyright (c) 2023 javiser
`crescendo-clock` is distributed under the terms of the MIT License.

See the [LICENSE](LICENSE) for license details.

