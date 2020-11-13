/* Hello World Example
   This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <esp_types.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "driver/gpio.h"

#define GPIO_BUTTON 0
#define GPIO_LED 2

void setup_gpio()
{
	gpio_config_t config_struct;

	config_struct.intr_type 	= GPIO_INTR_DISABLE;
	config_struct.mode			= GPIO_MODE_INPUT;
	config_struct.pin_bit_mask 	= (1ULL<<GPIO_BUTTON);
	config_struct.pull_down_en	= GPIO_PULLDOWN_DISABLE;
	config_struct.pull_up_en	= GPIO_PULLUP_ENABLE;

	gpio_config(&config_struct);

	config_struct.intr_type 	= GPIO_INTR_DISABLE;
	config_struct.mode			= GPIO_MODE_OUTPUT;
	config_struct.pin_bit_mask 	= (1ULL<<GPIO_LED);
	config_struct.pull_down_en	= GPIO_PULLDOWN_ENABLE;
	config_struct.pull_up_en	= GPIO_PULLUP_ENABLE;

	gpio_config(&config_struct);
}

void app_main(void)
{
    printf("Hello world!\n");

    setup_gpio();

    /* Print chip information */
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    printf("This is %s chip with %d CPU core(s), WiFi%s%s, ",
            CONFIG_IDF_TARGET,
            chip_info.cores,
            (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
            (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

    printf("silicon revision %d, ", chip_info.revision);

    printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
            (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    printf("Minimum free heap size: %d bytes\n", esp_get_minimum_free_heap_size());

    bool state = true;
    for (int i = 10; i >= 0; i--) {
        int level = gpio_get_level(GPIO_BUTTON);

        if(level)
        	state = !state;

        gpio_set_level(GPIO_LED, state);
        int read = gpio_get_level(GPIO_LED);

        printf("Restarting in %d seconds..., current switch state : %d %d\n", i, level, read);

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();
}
