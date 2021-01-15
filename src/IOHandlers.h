#ifndef __UNIVERSALREMOTE_TASKS__
#define __UNIVERSALREMOTE_TASKS__

#include "Arduino.h"

#define SHORT_BLINK_PER     100                                         // Time period for continuous blinking
#define SHORT_BLINK_TICKS   SHORT_BLINK_PER / portTICK_PERIOD_MS
#define LONG_BLINK_PER      500                                         // Time period for blinking once
#define LONG_BLINK_TICKS    LONG_BLINK_PER / portTICK_PERIOD_MS

#define LONG_PRESS_PER      5000                                        // Time period for which button has to be pressed to reset configuration
#define LONG_PRESS_TICKS    LONG_PRESS_PER / portTICK_PERIOD_MS

// Handler functions for FreeRTOS tasks
void blink_led_task(void* param);
void blink_led_once_task(void* param);
void button_read_task(void* param);

// For handling blinking of a single LED
class LedHandler
{
private:
    TaskHandle_t blinkOnceTask_h;
    TaskHandle_t blinkTask_h;

    int pin;

public:
    // @param pin_num   The number of pin connected to LED
    LedHandler(int pin_num, const char* blink_task_name, const char* blink_once_task_name);

    // Start continuously blinking LED
    void start_blinking();

    // Stop continuously blinking LED
    void stop_blinking();

    // Blink LED once
    void blink_once();

    // Turn LED on
    void on();

    // Turn LED off
    void off();
};

// Checks button status and resets configuration if required
class ResetHandler
{
private:
    TaskHandle_t buttonListenTask_h;

public:
    // @param pin_num   Number of pin connected to button
    ResetHandler(int pin_num);

    // Start the task
    void start();

    // Stop the task
    void stop();
};

#endif
