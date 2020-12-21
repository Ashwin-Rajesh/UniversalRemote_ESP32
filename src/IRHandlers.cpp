#include "IRHandlers.h"

ReceiveHandler::ReceiveHandler(int pin_num) : receiver(pin_num, kCaptureBufferSize, kTimeout, true)
{}

// Listens to the IR receiver pin, gets raw data and puts it into the passed string. 
// Returns ESP_FAIL if no signal is received.
esp_err_t ReceiveHandler::get_raw(String &str)
{
    receiver.enableIRIn();
    decode_results results;
    
    uint32_t now = millis();

    bool ir_recv = false;
    while((millis() - now) < kTimeoutReceive)
    {
        if(receiver.decode(&results))
        {
            ir_recv = true;
            break;
        }
    }

    receiver.disableIRIn();

    if(!ir_recv)
        return ESP_FAIL;
    
    str.reserve(MAX_STR_LEN);

    decode_type_t protocol = results.decode_type;

    ESP_LOGI(TAG, "%d", protocol);

    if(protocol > decode_type_t::kLastDecodeType)
        protocol = decode_type_t::UNKNOWN;
    
    if(protocol == -1)
        str += "-1;";
    else
        str += uint64ToString(results.decode_type) + ";";
    
    str += uint64ToString(results.rawlen) + ":";

    for (uint16_t i = 1; i < results.rawlen; i++) {
        uint32_t usecs;

        // Here, if a time cannot be shown as single 16 bit integer, it will be split into multiple parts.
        for (usecs = results.rawbuf[i] * kRawTick; usecs > UINT16_MAX;
            usecs -= UINT16_MAX) 
        {
            str += uint64ToString(UINT16_MAX);
            if (i % 2)
                str += F(", 0,  ");
            else
                str += F(",  0, ");
        }
        str += uint64ToString(usecs, 10);
        str += ",";            // ',' not needed on the last one
    }

    return ESP_OK;
}

SendHandler::SendHandler(int pin_num) : ac_sender(pin_num, false, true), sender(pin_num, false, true)
{
    pinMode(pin_num, OUTPUT);
}

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
esp_err_t SendHandler::send_ac(const char* str)
{
    std::string string;
    string = str;

    decode_type_t protocol;
    int16_t model;
    bool power;
    stdAc::opmode_t mode;
    float degrees; 
    bool celsius;
    stdAc::fanspeed_t fan;
    stdAc::swingv_t swingv; 
    stdAc::swingh_t swingh;
    bool quiet; 
    bool turbo;
    bool econo;
    bool light;
    bool filter;
    bool clean;
    bool beep;
    int16_t sleep; 
    int16_t clock;

    int prev_idx = 0;
    int curr_idx = 0;

    curr_idx = string.find(',', prev_idx);
    if(curr_idx == string.npos)
        return ESP_FAIL;
    protocol = (decode_type_t)atoi(string.substr(prev_idx, (curr_idx - prev_idx)).c_str());
    prev_idx = curr_idx+1;

    curr_idx = string.find(',', prev_idx);
    model = atoi(string.substr(prev_idx, (curr_idx - prev_idx)).c_str());
    prev_idx = curr_idx+1;

    curr_idx = string.find(',', prev_idx);
    power = (atoi(string.substr(prev_idx, (curr_idx - prev_idx)).c_str()) > 0);
    prev_idx = curr_idx+1;

    curr_idx = string.find(',', prev_idx);
    mode = (stdAc::opmode_t)atoi(string.substr(prev_idx, (curr_idx - prev_idx)).c_str());
    prev_idx = curr_idx+1;
    
    curr_idx = string.find(',', prev_idx);
    degrees = atof(string.substr(prev_idx, (curr_idx - prev_idx)).c_str());
    prev_idx = curr_idx+1;
    
    curr_idx = string.find(',', prev_idx);
    celsius = (atoi(string.substr(prev_idx, (curr_idx - prev_idx)).c_str()) > 0);
    prev_idx = curr_idx+1;
    
    curr_idx = string.find(',', prev_idx);
    fan = (stdAc::fanspeed_t)atoi(string.substr(prev_idx, (curr_idx - prev_idx)).c_str());
    prev_idx = curr_idx+1;

    curr_idx = string.find(',', prev_idx);
    swingv = (stdAc::swingv_t)atoi(string.substr(prev_idx, (curr_idx - prev_idx)).c_str());
    prev_idx = curr_idx+1;

    curr_idx = string.find(',', prev_idx);
    swingh = (stdAc::swingh_t)atoi(string.substr(prev_idx, (curr_idx - prev_idx)).c_str());
    prev_idx = curr_idx+1;
    
    curr_idx = string.find(',', prev_idx);
    quiet = (atoi(string.substr(prev_idx, (curr_idx - prev_idx)).c_str()) > 0);
    prev_idx = curr_idx+1;
        
    curr_idx = string.find(',', prev_idx);
    turbo = (atoi(string.substr(prev_idx, (curr_idx - prev_idx)).c_str()) > 0);
    prev_idx = curr_idx+1;
        
    curr_idx = string.find(',', prev_idx);
    econo = (atoi(string.substr(prev_idx, (curr_idx - prev_idx)).c_str()) > 0);
    prev_idx = curr_idx+1;
    
    curr_idx = string.find(',', prev_idx);
    light = (atoi(string.substr(prev_idx, (curr_idx - prev_idx)).c_str()) > 0);
    prev_idx = curr_idx+1;
    
    curr_idx = string.find(',', prev_idx);
    filter = (atoi(string.substr(prev_idx, (curr_idx - prev_idx)).c_str()) > 0);
    prev_idx = curr_idx+1;
    
    curr_idx = string.find(',', prev_idx);
    clean = (atoi(string.substr(prev_idx, (curr_idx - prev_idx)).c_str()) > 0);
    prev_idx = curr_idx+1;
    
    curr_idx = string.find(',', prev_idx);
    beep = (atoi(string.substr(prev_idx, (curr_idx - prev_idx)).c_str()) > 0);
    prev_idx = curr_idx+1;
    
    curr_idx = string.find(',', prev_idx);
    sleep = atoi(string.substr(prev_idx, (curr_idx - prev_idx)).c_str());
    prev_idx = curr_idx+1;
    
    curr_idx = string.find(',', prev_idx);
    clock = atoi(string.substr(prev_idx, (curr_idx - prev_idx)).c_str());
    prev_idx = curr_idx+1;

    ac_sender.sendAc(protocol, model, power, mode, degrees, celsius, fan,
              swingv, swingh,
              quiet, turbo, econo,
              light, filter, clean,
              beep, sleep, clock);
    
    return ESP_OK;
}

// Parses the string and sends
// Format : <number of raw timing entries>:<timing data seperated by comma>
// Sample : 10:8954,4180,540,1584,514,534,512,536,514,536
esp_err_t SendHandler::send_raw(const char* str)
{
    int rawlen = 0;
    std::string string(str);
    
    int prev_idx = string.find(':');
    int next_idx = 0;
    rawlen = atoi(string.substr(0, prev_idx).c_str());
    
    if(rawlen == 0)
        return ESP_FAIL;

    uint16_t *rawdata = (uint16_t*)malloc(sizeof(uint16_t)*rawlen);

    prev_idx += 1;
    for(int i = 0; i < rawlen; i++)
    {
        next_idx = string.find(',', prev_idx);
        std::string substr = string.substr(prev_idx, (next_idx-prev_idx));
        *(rawdata +i) = atoi(substr.c_str());
        prev_idx = next_idx + 1;
    }

    sender.sendRaw(rawdata, rawlen, 38);

    delete[] rawdata;

    return ESP_OK;
}
