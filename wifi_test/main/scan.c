/* Scan Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

/*
    This example shows how to scan for available set of APs.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"

#define MAX_AP_NUM 10

static const char *TAG = "scan";

void setup_wifi(void)
{
	wifi_init_config_t wifi_init_struct = WIFI_INIT_CONFIG_DEFAULT();

	esp_wifi_init(&wifi_init_struct);

	esp_wifi_set_mode(WIFI_MODE_STA);

	esp_wifi_start();
}

void wifi_scan(uint16_t *num, wifi_ap_record_t *records)
{

	esp_wifi_scan_start(NULL, true);

	esp_wifi_scan_get_ap_num(num);

	esp_wifi_scan_get_ap_records(num, records);
}

void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    setup_wifi();

    uint16_t num;
    wifi_ap_record_t accesspoints[MAX_AP_NUM];

    while(true)
    {
    	wifi_scan(&num, accesspoints);

    	printf(" %d Access points found. They are :\n", num);

    	for(int i = 0; i < num; i++)
    	{
    		printf(" %d : %s\n", (i+1), accesspoints[i].ssid);
    	}

    	int idx;
    	printf(" Which one would you like to connect to?");



    	printf(idx);

    	vTaskDelay(10000 / portTICK_PERIOD_MS);
    }

    esp_restart();
}
