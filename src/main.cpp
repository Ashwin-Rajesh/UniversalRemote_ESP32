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

#include "IOHandlers.h"
#include "IRHandlers.h"
#include "NetworkHandler.h"

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

#define TAG                     "main"

SendHandler sender(IR_SEND_PIN);
ReceiveHandler receiver(IR_RECV_PIN);

LedHandler IRled(GPIO_LED_IR, "IR blink", "IR blink once");
LedHandler WiFiled(GPIO_LED_WIFI, "WiFi blink", "WiFi blink once");

ResetHandler ResetButton(GPIO_RESET_BUTTON);

nvs_handle WiFiHandler::nvs_wifi;
LedHandler *WiFiHandler::WiFiled        = NULL;
LedHandler *WiFiHandler::IRled          = NULL;
SendHandler *WiFiHandler::sender        = NULL;
ReceiveHandler *WiFiHandler::receiver   = NULL;

void setup(){
    
    Serial.begin(115200);

    WiFiHandler networkManager(&WiFiled, &IRled, &sender, &receiver);

    if(networkManager.is_configured())
    {
#ifdef SOFT_CONFIG
        networkManager.auto_connect();
#else
        networkManager.force_connect(WIFI_SSID, WIFI_PASSWORD, MDNS_SERVICE_NAME);
#endif
    }
    else
    {
        networkManager.start_configure(WIFI_AP_SSID, WIFI_AP_PASSWORD);
    }
}

void loop()
{
    delay(1000);
}
