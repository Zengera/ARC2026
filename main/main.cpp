#include "RobotController.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

RobotController robot;

void setup()
{
    robot.begin();
}

void loop()
{
    robot.update();
}

extern "C" void app_main(void)
{
    setup();

    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(10); // 10ms 制御周期

    while (1) {
        loop();
        vTaskDelayUntil(&lastWakeTime, period);
    }
}
