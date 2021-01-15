#ifndef __UNIVERSALREMOTE_IR_
#define __UNIVERSALREMOTE_IR_

#include <Arduino.h>

#include <IRrecv.h>
#include <IRsend.h>
#include <IRutils.h>
#include <IRac.h>

// Maximum length of data expected for http server
#define MAX_STR_LEN 1500

// IR configuration parameters
const uint16_t kCaptureBufferSize = 1024;
const uint8_t kTimeout = 50;
const uint32_t kTimeoutReceive = 10000;
const uint16_t kMinUnknownSize = 12;
const uint8_t kTolerancePercentage = kTolerance;

class ReceiveHandler
{
private:
    IRrecv receiver;

public:
    // @param pin_num   The pin number to which the IR receiver has been connected
    ReceiveHandler(int pin_num);

    // Listens to the IR receiver pin, gets raw data and puts it into the passed string. 
    // Returns ESP_FAIL if no signal is received.
    esp_err_t get_raw(String &str);
};

class SendHandler
{
private:
    IRac ac_sender;
    IRsend sender;

public:
    // @param pin_num   The pin number to which the LED driver is connected
    SendHandler(int pin_num);

    // Parses the passed string and sends AC message
    // Format : protocol, model, power, mode, degrees, celsius, fan, swingv, swingh, quiet, turbo, econo, light, filter, clean, beep, sleep, clock
    // Sample : 10,1,1,1,25,1,2,4,2,1,0,1,1,0,0,1,-1,-1
    // Format explained
    // - protocol   - int(decode_type_t)    - The vendor/protocol type.
    // - model      - int                   - The A/C model if applicable.
    // - power      - bool                  - The power setting.
    // - mode       - int(opmode_t)         - The operation mode setting.
    // - degrees    - float                 - The temperature setting in degrees.
    // - celsius    - bool                  - Temperature units. True is Celsius, False is Fahrenheit.
    // - fan        - int(fanspeed_t)       - The speed setting for the fan.
    // The following are all "if supported" by the underlying A/C classes.
    // - swingv     - int(swingv_t)         - The vertical swing setting.
    // - swingh     - int(swingh_t)         - The horizontal swing setting.
    // - quiet      - bool                  - Run the device in quiet/silent mode.
    // - turbo      - bool                  - Run the device in turbo/powerful mode.
    // - econo      - bool                  - Run the device in economical mode.
    // - light      - bool                  - Turn on the LED/Display mode.
    // - filter     - bool                  - Turn on the (ion/pollen/etc) filter mode.
    // - clean      - bool                  - Turn on the self-cleaning mode. e.g. Mould, dry filters etc
    // - beep       - bool                  - Enable/Disable beeps when receiving IR messages.
    // - sleep      - int                   - Nr. of minutes for sleep mode.
    // - clock      - int                   - The time in Nr. of mins since midnight. < 0 is ignore.
    // Integers and floats are converted from string, and boolean is represented by integers (true for > 0, false otherwise)
    esp_err_t send_ac(const char* str);

    // Parses the string and sends
    // Format : <number of raw timing entries>:<timing data seperated by comma>
    // Sample : 10:8954,4180,540,1584,514,534,512,536,514,536
    esp_err_t send_raw(const char* str);
};

#endif