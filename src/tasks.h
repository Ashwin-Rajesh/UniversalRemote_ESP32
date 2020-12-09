#ifndef __UNIVERSALREMOTE_TASKS__
#define __UNIVERSALREMOTE_TASKS__

#include "tasks.h"
#include "Arduino.h"

#define GPIO_LED_WIFI       10
#define GPIO_LED_IR         11
#define GPIO_RESET_BUTTON   12

#define SHORT_BLINK_PER 100 / portTICK_PERIOD_MS
#define LONG_BLINK_PER  500 / portTICK_PERIOD_MS

TaskHandle_t blinkWiFiLED_h;
TaskHandle_t blinkOnceWiFiLED_h;
TaskHandle_t blinkIRLED_h;
TaskHandle_t blinkOnceIRLED_h;
TaskHandle_t resetConfigTask_h;

void blinkLedTask(void* param);
void blinkLedOnceTask(void* param);
void resetConfigTask(void* param);

#endif
