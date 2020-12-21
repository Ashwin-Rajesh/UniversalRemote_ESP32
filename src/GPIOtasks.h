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
    LedHandler(int pin);

    void start_blinking();

    void stop_blinking();

    void blink_once();

    void on();

    void off();
};

// Checks button status and resets configuration if required
class ResetHandler
{
private:
    TaskHandle_t buttonListenTask_h;

public:
    ResetHandler(int pint);

    void start();

    void stop();
};

#endif
