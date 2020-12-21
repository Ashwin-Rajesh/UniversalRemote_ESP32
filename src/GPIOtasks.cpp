#include "Arduino.h"

#include "GPIOtasks.h"

#include "nvs_flash.h"

#define TAG "gpio"

// Keeps blinking LED with period SHORT_BLINK_TICKS
void blink_led_task(void* param)
{
    static int pin = (int)param;
    pinMode(pin, OUTPUT);

    for(;;)
    {
        digitalWrite(pin, HIGH);
        vTaskDelay(SHORT_BLINK_TICKS);
        digitalWrite(pin, LOW);
        vTaskDelay(SHORT_BLINK_TICKS);
    }
}

// Blinks LED once for period LONG_BLINK_TICKS and then turns off
void blink_led_once_task(void* param)
{
    static int pin = (int)param;
    pinMode(pin, OUTPUT);

    for(;;)
    {
        digitalWrite(pin, HIGH);
        vTaskDelay(LONG_BLINK_TICKS);
        digitalWrite(pin, LOW);
        vTaskSuspend(NULL);
    }
}

// Keeps reading button input. If it is pressed for more than LONG_PRESS_TICKS, erase flash and initiate reset
void button_read_task(void* param)
{
    int pin = (int)param;
   
    vTaskSuspend(NULL);

    Serial.printf("button pin : %d", pin);

    for(;;)
    {
        if(!digitalRead(0))
        {
            Serial.printf("Restarting");
            nvs_flash_erase();
            esp_restart();
        }

        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

// Constructor for LedHandler
// @param : pin - Number of pin connected to LED
LedHandler::LedHandler(int pin_num)
{
    pin = pin_num;

    xTaskCreate(blink_led_task, "blink led", 1024, (void*)pin, 5, &blinkTask_h);
    vTaskSuspend(blinkTask_h);
    xTaskCreate(blink_led_once_task, "blink led once", 1024, (void*)pin, 5, &blinkOnceTask_h);
}

// Starts blinking
void LedHandler::start_blinking()
{
    ESP_LOGI("test", "START %d", pin);
    
    vTaskResume(blinkTask_h);
}

// Stop blinking
void LedHandler::stop_blinking()
{
    ESP_LOGI("test", "STOP %d", pin);
    
    vTaskSuspend(blinkTask_h);

    digitalWrite(pin, LOW);
}

// Blink once
void LedHandler::blink_once()
{
    ESP_LOGI("test", "BLINK ONCE %d", pin);
    vTaskResume(blinkOnceTask_h);
}

// Turn on
void LedHandler::on()
{
    ESP_LOGI("test", "ON %d", pin);

    digitalWrite(pin, HIGH);
}

// Turn off
void LedHandler::off()
{
    ESP_LOGI("test", "OFF %d", pin);
    
    digitalWrite(pin, LOW);
}

// Constructor for ResetHandler
// @param : pin - Number of pin connected to the button
ResetHandler::ResetHandler(int pin_num)
{
    pinMode(0, INPUT);

    ESP_LOGI("test", "BUTTON %d", pin_num);

    xTaskCreate(button_read_task, "config reset", 1024, (void *)0, 5, &buttonListenTask_h);
}

// Start reset task
void ResetHandler::start()
{
    vTaskResume(buttonListenTask_h);
}

// Stop reset task
void ResetHandler::stop()
{
    vTaskSuspend(buttonListenTask_h);
}
