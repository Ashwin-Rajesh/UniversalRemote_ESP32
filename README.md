# UniversalRemote_ESP32
An IOT universal remote controller using the ESP32 microcontroller (ESP32-WROOM32 module), with firmware developed using the Arduino framework.

---

## Hardware used
1. ESP32S NodeMCU development board
2. TSOP1838 IR receiver
3. Breadboard
4. BC547 transistor
5. IR LED x 3

---

## Tools / Software frameworks used
1. [Arduino framework for ESP32](https://github.com/espressif/arduino-esp32)
2. [Platformio for VS Code](https://marketplace.visualstudio.com/items?itemName=platformio.platformio-ide)

---

## Abstract
The purpose of this device is to act as a bridge between the digital world and IR controlled devices. The device can connect to a WiFi network and hosts an HTTP server. Here are the URI that can be used to communicate with the server:

### 1. GET "/"
This reteurns raw timing data of an IR signal that is captured by its on-board TSOP1838 sensor. The timing data has the format :
```<number of raw timing entries>:<timing data seperated by comma>```
For example,
```10:8954,4180,540,1584,514,534,512,536,514,536```