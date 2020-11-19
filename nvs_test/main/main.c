/* Non-Volatile Storage (NVS) Read and Write a Blob - Example

   For other examples please check:
   https://github.com/espressif/esp-idf/tree/master/examples

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "driver/gpio.h"

#define STORAGE_NAMESPACE "storage"

/* Save the number of module restarts in NVS
   by first reading and then incrementing
   the number that has been saved previously.
   Return an error if anything goes wrong
   during this process.
 */

// Setup GPIO
void setup_gpio(void)
{
	gpio_config_t gpio_struct = {
		.intr_type  = GPIO_INTR_DISABLE,
		.mode		= GPIO_MODE_INPUT,
		.pin_bit_mask 	= 1ULL << 0,
		.pull_down_en 	= GPIO_PULLDOWN_ENABLE,
		.pull_up_en 	= GPIO_PULLUP_ENABLE
	};

	gpio_config(&gpio_struct);
}

// Setup non-volatile storage
void setup_nvs()
{
	esp_err_t err = nvs_flash_init();

	if(err != ESP_OK)
	{
		ESP_LOGW("nvs_test", " Somethings wrong. Clearing flash and resetting...");
		ESP_ERROR_CHECK(nvs_flash_erase());
		vTaskDelay(1000 / portTICK_PERIOD_MS);

		esp_restart();
	}


}

// Increments value called "count" in memory
void inc_count()
{
	nvs_handle_t nvs_obj;

	nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &nvs_obj);

	int16_t value = 0;
	nvs_get_i16(nvs_obj, "count", &value);

	value++;

	nvs_set_i16(nvs_obj, "count", value);

	nvs_commit(nvs_obj);

	ESP_LOGI("nvs_test", " Incremented value : %d", value);

	nvs_close(nvs_obj);
}

// Task to check button (GPIO0) and if pressed, call increment function
void inc_task(void *param)
{
	while(true)
	{
		if(!gpio_get_level(GPIO_NUM_0))
			inc_count();

		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}

void app_main(void)
{

	setup_nvs();
	ESP_LOGI("nvs_test", "Setup NVS");

	setup_gpio();
	ESP_LOGI("nvs_test", "Setup GPIO");


	TaskHandle_t incHandle;
	xTaskCreate(&inc_task, "increment", 2048, NULL, 1, &incHandle);
}
