/* 
* Copyright (c) 2020 Ashwin Rajesh.
* 
* This program is free software: you can redistribute it and/or modify  
* it under the terms of the GNU General Public License as published by  
* the Free Software Foundation, version 3.
*
* This program is distributed in the hope that it will be useful, but 
* WITHOUT ANY WARRANTY; without even the implied warranty of 
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License 
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <Arduino.h>
#include "WiFi.h"
#include "ESPmDNS.h"

#include <esp_http_server.h>

#include <nvs.h>
#include <nvs_flash.h>

#include <IRrecv.h>
#include <IRsend.h>
#include <IRac.h>
#include <IRutils.h>

#include "GPIOtasks.h"

// Maximum length of data expected for http server
#define MAX_STR_LEN 1500

// GPIO settings
#define GPIO_LED_WIFI       4
#define GPIO_LED_IR         2
#define GPIO_RESET_BUTTON   5

#define IR_RECV_PIN         15
#define IR_SEND_PIN         14

// NVS namespace, ssid and password keys
#define NVS_NAMESPACE           "wifiConfig"
#define NVS_SSID_KEY            "ssid"
#define NVS_PASSWORD_KEY        "password"
#define NVS_HOSTNAME_KEY        "hostname"

// http server url's
#define HTTP_RAW_SEND_URI       "/"
#define HTTP_GET_URI            "/"
#define HTTP_AC_SEND_URI        "/ac"
#define HTTP_WIFI_SCAN_URI      "/scan"
#define HTTP_WIFI_CONFIG_URI    "/wificonfig"

// wifi IP address in configuration phase
#define WIFI_CONFIG_IP          "192.168.1.1"
#define WIFI_TIMEOUT            10

// Wifi settings
#define SOFT_CONFIG             // Comment this for using hardcoded SSID, password, and hostname data. Otherwise, they will be stored in flash memory

#define WIFI_AP_SSID            "UniversalIRBlaster"
#define WIFI_AP_PASSWORD        "test12345678"

#define WIFI_SSID               "test"
#define WIFI_PASSWORD           "12345678"
#define MDNS_SERVICE_NAME       "UniversalRemoteTest"

// IR configuration parameters
const uint16_t kCaptureBufferSize = 1024;
const uint8_t kTimeout = 50;
const uint32_t kTimeoutReceive = 10000;
const uint16_t kMinUnknownSize = 12;
const uint8_t kTolerancePercentage = kTolerance;

nvs_handle nvs_wifi;

IRrecv receiver(IR_RECV_PIN, kCaptureBufferSize, kTimeout, true);
IRsend sender(IR_SEND_PIN, false, true);
IRac ac_sender(IR_SEND_PIN, false, true);

LedHandler IRled(GPIO_LED_IR);
LedHandler WiFiled(GPIO_LED_WIFI);

ResetHandler ResetButton(GPIO_RESET_BUTTON);

// Listens to the IR receiver pin, gets raw data and puts it into the passed string. 
// Returns ESP_FAIL if no signal is received.
esp_err_t getRaw(String &str)
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

// Handler function for http get requests for raw messages
esp_err_t http_get_handler(httpd_req_t* req)
{
    String resp;

	ESP_LOGI(TAG, " Got a get request.");

    IRled.on();
    esp_err_t ret = getRaw(resp);
    IRled.off();

    if(ret == ESP_FAIL)
        resp = "-1";

    ESP_LOGI(TAG, "Sending response :%s", resp.c_str());

	httpd_resp_send(req, resp.c_str(), resp.length());

	return ESP_OK;
}

// Parses the string and sends
// Format : <number of raw timing entries>:<timing data seperated by comma>
// Sample : 10:8954,4180,540,1584,514,534,512,536,514,536
esp_err_t sendRaw(const char* str)
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

// Handler function for http post messages for raw messages
esp_err_t http_post_handler(httpd_req_t *req)
{
    /* Destination buffer for content of HTTP POST request.
     * httpd_req_recv() accepts char* only, but content could
     * as well be any binary data (needs type casting).
     * In case of string data, null termination will be absent, and
     * content length would give length of string */
    char content[MAX_STR_LEN];

    /* Truncate if content length larger than the buffer */
    size_t recv_size = req->content_len;
    if(recv_size < sizeof(content)) recv_size = sizeof(content);

    int ret = httpd_req_recv(req, content, recv_size); 

    if (ret <= 0) {  /* 0 return value indicates connection closed */
        /* Check if timeout occurred */
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            /* In case of timeout one can choose to retry calling
             * httpd_req_recv(), but to keep it simple, here we
             * respond with an HTTP 408 (Request Timeout) error */
            httpd_resp_send_408(req);
        }
        /* In case of error, returning ESP_FAIL will
         * ensure that the underlying socket is closed */
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Got a post request to / :%s", content);

    IRled.blink_once();
    
    esp_err_t str_ret = sendRaw(content);
    
    char resp[100];
    
    // Send response to client
    if(str_ret == ESP_FAIL)
        strcpy(resp, "Invalid format");
    else
        strcpy(resp, "Success");

    ESP_LOGI(TAG, "Sending response :%s", resp);
    
    httpd_resp_send(req, resp, strlen(resp));

    return ESP_OK;
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
esp_err_t sendAC(const char* str)
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

// Handler function for http post message type, for AC messages
esp_err_t http_ac_post_handler(httpd_req_t *req)
{
    char content[MAX_STR_LEN];

    size_t recv_size = req->content_len;
    if(recv_size < sizeof(content)) recv_size = sizeof(content);

    esp_err_t ret = httpd_req_recv(req, content, recv_size);

    if (ret <= 0) 
    {
        // Send a request timed out error code (408) if connection timedout
        if (ret == HTTPD_SOCK_ERR_TIMEOUT)
            httpd_resp_send_408(req);
        // To close the socket, return ESP_FAIL
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Got a post request to /ac :%s", content);

    esp_err_t str_ret = sendAC(content);
    
    char resp[100];
    
    // Send response to client
    if(str_ret == ESP_FAIL)
        strcpy(resp, "Invalid format");
    else
        strcpy(resp, "Success");

    ESP_LOGI(TAG, "Sending response :%s", resp);
    
    httpd_resp_send(req, resp, strlen(resp));

    return ESP_OK;
}

// Scans for available wifi networks and 
esp_err_t http_scan_handler(httpd_req_t *req)
{
    int n = WiFi.scanNetworks();
        if (n == 0) {
        ESP_LOGI(TAG, "no networks found");
    } else {
        ESP_LOGI(TAG, "%d", n);
        ESP_LOGI(TAG, " networks found");
        for (int i = 0; i < n; ++i) {
            // Print SSID and RSSI for each network found
            ESP_LOGI(TAG, "%d : %s", i, WiFi.SSID(i));
            delay(10);
        }
    }
    
    String response;
    response.reserve(MAX_STR_LEN);

    for(int i = 0; i < n; ++i) {
        response += WiFi.SSID(i) + "$";
    }
    
    httpd_resp_send(req, response.c_str(), response.length());

    return ESP_OK;
}

esp_err_t configWiFi(const char* str)
{
    std::string ssid;
    std::string hostname;
    std::string password;

    std::string content;

    content = str;

    int previdx = 0;
    int curridx = 0;

    ESP_LOGI(TAG, "%s", content.c_str());

    curridx = content.find('$', previdx);
    hostname = content.substr(previdx, (curridx-previdx));
    previdx = curridx + 1;

    curridx = content.find('$', previdx);
    ssid = content.substr(previdx, (curridx-previdx));
    previdx = curridx + 1;
    
    curridx = content.find('$', previdx);
    password = content.substr(previdx, (curridx-previdx));
    previdx = curridx + 1;

    WiFi.softAPdisconnect();
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    WiFi.begin(ssid.c_str(), password.c_str());

    ESP_LOGI(TAG, "%s|%s|%s", hostname.c_str(), ssid.c_str(), password.c_str());

    long now = millis();

    WiFiled.start_blinking();
    
    while(WiFi.status() != WL_CONNECTED && (millis() - now < WIFI_TIMEOUT*1000))
    {   
        if(WiFi.status() == WL_NO_SSID_AVAIL)
        {
            ESP_LOGI(TAG, "Connection failed");
            return ESP_FAIL;
        }
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }

    WiFiled.stop_blinking();

    if(WiFi.status() == WL_CONNECTED)
    {
        ESP_LOGI(TAG, "Connected successfully");

        nvs_set_str(nvs_wifi, NVS_HOSTNAME_KEY, hostname.c_str());

        nvs_set_str(nvs_wifi, NVS_SSID_KEY, ssid.c_str());

        nvs_set_str(nvs_wifi, NVS_PASSWORD_KEY, password.c_str());

        nvs_commit(nvs_wifi);
        esp_restart();
        return ESP_OK;
    }
    else
    {
        ESP_LOGI(TAG, "Connection failed");
        esp_restart();
        return ESP_FAIL;
    }    
}

// For configuring the device
// Format  : "<hostname>$<SSID>$<password>$"
// Example : "Living Room$myWiFi$really Strong Password$"
esp_err_t http_config_handler(httpd_req_t *req)
{
    char content[MAX_STR_LEN];

    /* Truncate if content length larger than the buffer */
    size_t recv_size = req->content_len;
    if(recv_size < sizeof(content)) recv_size = sizeof(content);

    int ret = httpd_req_recv(req, content, recv_size); 
        if (ret <= 0) {  /* 0 return value indicates connection closed */
        /* Check if timeout occurred */
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            /* In case of timeout one can choose to retry calling
             * httpd_req_recv(), but to keep it simple, here we
             * respond with an HTTP 408 (Request Timeout) error */
            httpd_resp_send_408(req);
        }
        /* In case of error, returning ESP_FAIL will
         * ensure that the underlying socket is closed */
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Got a post request to /ac :%s", content);
    
    std::string response = "Got request";

    httpd_resp_send(req, response.c_str(), response.length());

    configWiFi(content);

    return ESP_OK;
}

// Starts the webserver for configuring the device (WiFi SSID, password and mDNS hostname)
httpd_handle_t start_config_webserver(void) 
{
    httpd_uri_t uri_scan;
    uri_scan.handler = &http_scan_handler;
    uri_scan.method  = HTTP_GET;
    uri_scan.uri = HTTP_WIFI_SCAN_URI;
    uri_scan.user_ctx = NULL;

    httpd_uri_t uri_config;
    uri_config.handler = &http_config_handler;
    uri_config.method  = HTTP_POST;
    uri_config.uri = HTTP_WIFI_CONFIG_URI;
    uri_config.user_ctx = NULL;

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) {
        /* Register URI handlers */
        httpd_register_uri_handler(server, &uri_scan);
        httpd_register_uri_handler(server, &uri_config);        
    }

    return server;
}

// Starts the webserver for handling IR requests
httpd_handle_t start_ir_webserver(void)
{
	httpd_uri_t uri_get;
    uri_get.handler = &http_get_handler;
    uri_get.method  = HTTP_GET;
    uri_get.uri = HTTP_GET_URI;
    uri_get.user_ctx = NULL;

	httpd_uri_t uri_post;
    uri_post.handler = &http_post_handler;
    uri_post.method  = HTTP_POST;
    uri_post.uri = HTTP_RAW_SEND_URI;
    uri_post.user_ctx = NULL;

    httpd_uri_t uri_ac_post;
    uri_ac_post.handler = &http_ac_post_handler;
    uri_ac_post.method  = HTTP_POST;
    uri_ac_post.uri = HTTP_AC_SEND_URI;
    uri_ac_post.user_ctx = NULL;

    httpd_uri_t uri_scan;
    uri_scan.handler = &http_scan_handler;
    uri_scan.method  = HTTP_GET;
    uri_scan.uri = HTTP_WIFI_SCAN_URI;
    uri_scan.user_ctx = NULL;

    /* Generate default configuration */
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    /* Empty handle to esp_http_server */
    httpd_handle_t server = NULL;

    /* Start the http server */
    if (httpd_start(&server, &config) == ESP_OK) {
        /* Register URI handlers */
        httpd_register_uri_handler(server, &uri_get);
        httpd_register_uri_handler(server, &uri_post);
        httpd_register_uri_handler(server, &uri_ac_post);    
        httpd_register_uri_handler(server, &uri_scan);    
    }
    /* If server failed to start, handle will be NULL */
    return server;
}

void stop_webserver(httpd_handle_t* server)
{
	httpd_stop(server);
}

httpd_handle_t server;

void setup(){

    pinMode(IR_SEND_PIN, OUTPUT);

    Serial.begin(115200);

    WiFiled.start_blinking();
    
#ifdef SOFT_CONFIG                          // Comment SOFT_CONFIG definition if you are using pre-defined SSID and password
    nvs_flash_init();

    nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_wifi);

    char* ssid;
    size_t ssid_len;

    char* password;
    size_t password_len;

    char* hostname;
    size_t hostname_len;

    bool setup = nvs_get_str(nvs_wifi, NVS_SSID_KEY, NULL, NULL) == ESP_ERR_NVS_NOT_FOUND;

    if(setup) {
        ESP_LOGI(TAG, "Configuration not done. Configuration mode activated");
        WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD);
        IPAddress myIP = WiFi.softAPIP();
        Serial.print("AP IP address: ");
        Serial.println(myIP);

        start_config_webserver();

        return;
    }
    else {
        ESP_LOGI(TAG, " Configuration detected.");

        ResetButton.start();
        
        nvs_get_str(nvs_wifi, NVS_SSID_KEY, NULL, &ssid_len);
        ssid = (char*)malloc(ssid_len);
        nvs_get_str(nvs_wifi, NVS_SSID_KEY, ssid, &ssid_len);

        nvs_get_str(nvs_wifi, NVS_PASSWORD_KEY, NULL, &password_len);
        password = (char*)malloc(password_len);
        nvs_get_str(nvs_wifi, NVS_PASSWORD_KEY, password, &password_len);
        
        nvs_get_str(nvs_wifi, NVS_HOSTNAME_KEY, NULL, &hostname_len);
        hostname = (char*)malloc(hostname_len);
        nvs_get_str(nvs_wifi, NVS_HOSTNAME_KEY, hostname, &hostname_len);

        ESP_LOGI(TAG, "%s|%s|%s", hostname, ssid, password);
    }
#else

    char* ssid = WIFI_AP_SSID;
    char* password = WIFI_AP_PASSWORD;

    char* hostname = MDNS_SERVICE_NAME;

#endif

    receiver.setUnknownThreshold(kMinUnknownSize);
    receiver.setTolerance(kTolerancePercentage);

    sender.begin();

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    WiFi.begin(ssid, password);

    
    while(WiFi.status() != WL_CONNECTED)
    {
        vTaskDelay(1000);
    }
    
    WiFiled.stop_blinking();

    Serial.println("WiFi Setup done. Setting up server");
    
    server = start_ir_webserver();

    Serial.println("Server setup, starting mDNS");

    WiFiled.on();

    Serial.println(MDNS.begin(hostname));

    MDNS.addService("http", "tcp", 80);

    MDNS.addServiceTxt("http", "tcp", "test", "100");

    WiFiled.off();

    Serial.println("Done");
}

void loop(){
    // if(!digitalRead(GPIO_BUTTON))
    // {
    //     ESP_LOGI(TAG, "Erasing nvs");
    //     nvs_flash_erase();
    //     esp_restart();
    // }
    // delay(2000);
}