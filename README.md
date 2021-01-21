# UniversalRemote_ESP32
An IOT universal remote controller using the ESP32 microcontroller (ESP32-WROOM32 module), with firmware developed using the Arduino framework. The companion android application : [Universal remote android application](https://github.com/harithmanoj/Universal-IR-Remote-Android/tree/main)

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

#### 1. GET "/"
This reteurns raw timing data of an IR signal that is captured by its on-board TSOP1838 sensor. The timing data has the format :

```<protocol detected>;<number of raw timing entries>:<timing data seperated by comma>```

For example,

```-1;10:8954,4180,540,1584,514,534,512,536,514,536```

#### 2. POST "/"
This sends the data in the request payload part which is assumed to be in the same raw format as received by the GET request (barring the protocol). The format is

```<number of raw timing entries>:<timing data seperated by comma>```

For example, 

```10:8954,4180,540,1584,514,534,512,536,514,536```

#### 3. GET "/scan"
This returns the wifi networks that the ESP32 can see, after executing a scan. The SSIDs of the networks are returned, seperated by '$'.

Example:

```my Wifi$neighbours Wifi$test Wifi$```

#### 4. POST "/ac"
Air conditioners have an exception because the IR signals they send encode information like temperature and swing. This means that the signal will not be the same for every time we press a button, and so, we need a different approach than storing the signal corresponding to each button and then sending it back. For this, we have created a seperate API for air conditioners. The information passed to this will be of the format

```protocol, model, power, mode, degrees, celsius, fan, swingv, swingh, quiet, turbo, econo, light, filter, clean, beep, sleep, clock```

For example, 

```10,1,1,1,25,1,2,4,2,1,0,1,1,0,0,1,-1,-1```

Format explained
 - protocol   - int(decode_type_t)    - The vendor/protocol type.
 - model      - int                   - The A/C model if applicable.
 - power      - bool                  - The power setting.
 - mode       - int(opmode_t)         - The operation mode setting.
 - degrees    - float                 - The temperature setting in degrees.
 - celsius    - bool                  - Temperature units. True is Celsius, False is Fahrenheit.
 - fan        - int(fanspeed_t)       - The speed setting for the fan.
 The following are all "if supported" by the underlying A/C classes.
 - swingv     - int(swingv_t)         - The vertical swing setting.
 - swingh     - int(swingh_t)         - The horizontal swing setting.
 - quiet      - bool                  - Run the device in quiet/silent mode.
 - turbo      - bool                  - Run the device in turbo/powerful mode.
 - econo      - bool                  - Run the device in economical mode.
 - light      - bool                  - Turn on the LED/Display mode.
 - filter     - bool                  - Turn on the (ion/pollen/etc) filter mode.
 - clean      - bool                  - Turn on the self-cleaning mode. e.g. Mould, dry filters etc
 - beep       - bool                  - Enable/Disable beeps when receiving IR messages.
 - sleep      - int                   - Nr. of minutes for sleep mode.
 - clock      - int                   - The time in Nr. of mins since midnight. < 0 is ignore.

Additionally, we also have a configuration stage, where we can send WiFi connection details and the mDNS service name to the ESP32. The data via the API:

#### 5. POST "/wificonfig"
This is available only during configuration stage. This stage is active only when the device has not been configured before, or if it has been reset by long pressing EN button.

The data sent is of the format :

```"<hostname>$<SSID>$<password>$"```

Example:

```Living Room$myWiFi$really Strong Password$```
