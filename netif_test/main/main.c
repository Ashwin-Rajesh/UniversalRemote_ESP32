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
#include "driver/gpio.h"

#define MAX_AP_NUM 10
#define BUF_SIZE (1024)

#define TAG "wifi_connect"

#define WIFI_SSID "FTTH"
#define WIFI_PASSWORD "12345678"

static void wifi_event_handler(void* event_handler, esp_event_base_t event, int32_t event_id, void* event_data);

static void got_ip_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

void setup_wifi(void);

void setup_gpio(void);

void wifi_scan(uint16_t *num, wifi_ap_record_t *records);

void wifi_connect(void);

esp_netif_t* netif_obj;

void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    esp_event_loop_create_default();

    esp_netif_init();

    netif_obj = esp_netif_create_default_wifi_sta();

    setup_wifi();

    setup_gpio();


    uint16_t num;
    wifi_ap_record_t accesspoints[MAX_AP_NUM];

    wifi_scan(&num, accesspoints);

    ESP_LOGI(TAG, " %d Access points found. They are :\n", num);

    for(int i = 0; i < num; i++)
    {
    	ESP_LOGI(TAG, " %d : %s\n", (i+1), accesspoints[i].ssid);
    }

    wifi_connect();

    vTaskDelay(2000/portTICK_PERIOD_MS);

    esp_netif_dhcpc_stop(netif_obj);

    esp_netif_ip_info_t ip_info;

    esp_netif_get_ip_info(netif_obj, &ip_info);

    esp_netif_set_ip4_addr(&ip_info.ip, 192, 168, 1, 100);

    ESP_LOGI(TAG, "Trying to set IP address " IPSTR, IP2STR(&ip_info.ip));

    esp_netif_set_ip_info(netif_obj, &ip_info);

    bool state = true;

    while(true)
    {
    	gpio_set_level(GPIO_NUM_2, state);

    	vTaskDelay(1000 / portTICK_PERIOD_MS);

    	state = !state;
    }
    esp_restart();
}

static void wifi_event_handler(void* event_handler, esp_event_base_t event, int32_t event_id, void* event_data)
{
	wifi_ap_record_t ap_record;

	switch(event_id)
	{
	case WIFI_EVENT_STA_CONNECTED:

		esp_wifi_sta_get_ap_info(&ap_record);

		ESP_LOGI(TAG, " Station connected to SSID : %s", ap_record.ssid);

		break;

	case WIFI_EVENT_STA_WPS_ER_FAILED:

		ESP_LOGI(TAG, " WPS authentication failed");

		break;

	case WIFI_EVENT_STA_WPS_ER_TIMEOUT:

		ESP_LOGI(TAG, " WPS authentication timed out");

		break;

	case WIFI_EVENT_STA_WPS_ER_SUCCESS:

		esp_wifi_sta_get_ap_info(&ap_record);

		ESP_LOGI(TAG, " Station connected to SSID : %s", ap_record.ssid);

		break;

	default:
		ESP_LOGI(TAG, "Some other %d event was detected", event_id);
	}
}



static void got_ip_event_handler(void* arg, esp_event_base_t event_base,
                             int32_t event_id, void* event_data)
{
    esp_netif_ip_info_t ip_info;

    esp_netif_get_ip_info(netif_obj, &ip_info);

    ESP_LOGI(TAG, "IP : " IPSTR " Gateway : " IPSTR " Netmask : " IPSTR, IP2STR(&ip_info.ip), IP2STR(&ip_info.gw), IP2STR(&ip_info.netmask));

    vTaskDelay(1000 / portTICK_PERIOD_MS);
}

void setup_wifi(void)
{
	wifi_init_config_t wifi_init_struct = WIFI_INIT_CONFIG_DEFAULT();

	esp_wifi_init(&wifi_init_struct);

	esp_wifi_set_mode(WIFI_MODE_STA);

	esp_wifi_start();
}

void setup_gpio(void)
{
	gpio_config_t gpio_struct = {
		.intr_type = GPIO_INTR_DISABLE,
		.mode = GPIO_MODE_OUTPUT,
		.pin_bit_mask = 1ULL << GPIO_NUM_2,
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
		.pull_up_en = GPIO_PULLUP_DISABLE
	};

	gpio_config(&gpio_struct);
}

void wifi_scan(uint16_t *num, wifi_ap_record_t *records)
{
	esp_wifi_scan_start(NULL, true);

	esp_wifi_scan_get_ap_num(num);

	esp_wifi_scan_get_ap_records(num, records);
}

void wifi_connect(void)
{
    wifi_config_t wifi_struct = {
    		.sta = {
    				.ssid = WIFI_SSID,
		            .password = WIFI_PASSWORD,
		            .bssid_set = false
    		}
    };


    ESP_LOGI(TAG, " Trying to connect. SSID : %s, Password : %s", wifi_struct.sta.ssid, wifi_struct.sta.password);

	esp_wifi_set_config(WIFI_IF_STA, &wifi_struct);

	ESP_ERROR_CHECK(esp_wifi_connect());

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &got_ip_event_handler, NULL));
}
