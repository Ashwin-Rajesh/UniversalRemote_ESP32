#ifndef __UNIVERSALREMOTE_NETWORK__
#define __UNIVERSALREMOTE_NETWORK__

#include <Arduino.h>
#include <WiFi.h>

#include <esp_http_server.h>

#include <nvs.h>

#include <IRHandlers.h>
#include <IOHandlers.h>

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

class WiFiHandler
{
private:
    static nvs_handle nvs_wifi;
    
    static esp_err_t http_get_handler(httpd_req_t* req);
    static esp_err_t http_post_handler(httpd_req_t *req);
    static esp_err_t http_ac_post_handler(httpd_req_t *req);

    static esp_err_t http_scan_handler(httpd_req_t *req);
    static esp_err_t http_config_handler(httpd_req_t *req);

    static esp_err_t config_network(const char* str);
    static esp_err_t connect_to_network(const char* ssid,const char* password);
    static esp_err_t start_mdns(const char* hostname);

    bool mode;

    static LedHandler *WiFiled;
    static LedHandler *IRled;

    static SendHandler *sender;
    static ReceiveHandler *receiver;

public:
    WiFiHandler(LedHandler *wifi, LedHandler *ir, SendHandler *send, ReceiveHandler *recv);
    
    bool is_configured();

    bool is_connected();

    esp_err_t start_configure(const char* ap_ssid, const char* ap_password);

    esp_err_t auto_connect();

    esp_err_t force_connect(const char* ssid, const char* password, const char* hostname);
};

#endif      // __UNIVERSALREMOTE_NETWORK__