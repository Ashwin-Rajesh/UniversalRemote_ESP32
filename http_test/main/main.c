#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include <esp_http_server.h>
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define TAG "HTTPS_TEST"

#define WIFI_SSID 		"FTTH"
#define WIFI_PASSWORD 	"12345678"

#define mDNS_NAME 		"IR_adapter_front"

#define GPIO_LED 		GPIO_NUM_2
#define GPIO_BUTTON		GPIO_NUM_0

esp_netif_t *netif_obj;

void setup_gpio(void)
{
	gpio_config_t gpio_conf = {
		.intr_type		= GPIO_INTR_DISABLE,
		.mode			= GPIO_MODE_OUTPUT,
		.pin_bit_mask	= 1ULL << GPIO_LED,
		.pull_down_en	= GPIO_PULLDOWN_DISABLE,
		.pull_up_en		= GPIO_PULLUP_DISABLE
	};

	gpio_config(&gpio_conf);

	gpio_conf.mode 			= GPIO_MODE_INPUT;
	gpio_conf.pin_bit_mask	= 1ULL << GPIO_BUTTON;

	gpio_config(&gpio_conf);
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

	default:
		ESP_LOGI(TAG, "Some other %d event was detected", event_id);
	}
}

static void got_ip_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    esp_netif_ip_info_t ip_info;

    esp_netif_get_ip_info(netif_obj, &ip_info);

    ESP_LOGI(TAG, "IP : " IPSTR " Gateway : " IPSTR " Netmask : " IPSTR, IP2STR(&ip_info.ip), IP2STR(&ip_info.gw), IP2STR(&ip_info.netmask));

    vTaskDelay(1000 / portTICK_PERIOD_MS);
}

void setup_wifi(void)
{
	esp_event_loop_create_default();

	ESP_ERROR_CHECK(esp_netif_init());

	netif_obj = esp_netif_create_default_wifi_sta();

	wifi_init_config_t wifi_conf = WIFI_INIT_CONFIG_DEFAULT();

	ESP_ERROR_CHECK(esp_wifi_init(&wifi_conf));

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

	ESP_ERROR_CHECK(esp_wifi_start());

	wifi_config_t sta_conf = {
		.sta = {
			.ssid 		= WIFI_SSID,
			.password 	= WIFI_PASSWORD,
			.bssid_set  = false
		}
	};

	ESP_LOGI(TAG, " Trying to connect. SSID : %s, Password : %s", sta_conf.sta.ssid, sta_conf.sta.password);

	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_conf));

	ESP_ERROR_CHECK(esp_wifi_connect());

	ESP_LOGI(TAG, "Finished setting up WiFi with SSID : %s", WIFI_SSID);

	ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
	ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &got_ip_event_handler, NULL));
}

bool level = 0;
esp_err_t http_get_handler(httpd_req_t* req)
{
	const char* resp = "Hello there!";

	httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);

	level = !level;

	gpio_set_level(GPIO_LED, level);

	ESP_LOGI(TAG, " %d Got a request : %s", level, req->uri);

	return ESP_OK;
}

httpd_handle_t start_webserver(void)
{

	httpd_uri_t uri_get = {
			.handler 	= &http_get_handler,
			.method		= HTTP_GET,
			.uri		= "/led",
			.user_ctx	= NULL
	};

    /* Generate default configuration */
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    /* Empty handle to esp_http_server */
    httpd_handle_t server = NULL;

    /* Start the http server */
    if (httpd_start(&server, &config) == ESP_OK) {
        /* Register URI handlers */
        httpd_register_uri_handler(server, &uri_get);
    }
    /* If server failed to start, handle will be NULL */
    return server;
}

void stop_webserver(httpd_handle_t* server)
{
	httpd_stop(server);
}

void app_main(void)
{
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
	        ESP_ERROR_CHECK(nvs_flash_erase());
	        ret = nvs_flash_init();
	}

    setup_gpio();

    setup_wifi();

    httpd_handle_t server = start_webserver();

    while (true) {
        	vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

