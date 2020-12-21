#ifndef __UNIVERSALREMOTE_NETWORK__
#define __UNIVERSALREMOTE_NETWORK__

#include <Arduino.h>
#include <WiFi.h>

#include <esp_http_server.h>

#include <nvs.h>

#include <IRHandlers.h>
#include <GPIOtasks.h>

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

#define WIFI_SSID               "OPTERNA-7D26"
#define WIFI_PASSWORD           "12345678"
#define MDNS_SERVICE_NAME       "UniversalRemoteTest"

class WiFiHandler
{
private:
    nvs_handle nvs_wifi;
    
    esp_err_t http_get_handler(httpd_req_t* req);
    esp_err_t http_post_handler(httpd_req_t *req);
    esp_err_t http_ac_post_handler(httpd_req_t *req);

    esp_err_t http_scan_handler(httpd_req_t *req);
    esp_err_t http_config_handler(httpd_req_t *req);

    esp_err_t config_network(const char* str);

    bool mode;

    LedHandler &WiFiled;
    LedHandler &IRled;

    SendHandler &sender;
    ReceiveHandler &receiver;

public:
    WiFiHandler(LedHandler& wifi, LedHandler &ir, SendHandler &send, ReceiveHandler &recv);

    bool is_configured();

    bool is_connected();

    esp_err_t start_configure(const char* ap_ssid, const char* ap_password);

    esp_err_t start_connect();
};

#endif      // __UNIVERSALREMOTE_NETWORK__