#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern "C" void vApplicationStackOverflowHook(TaskHandle_t xTask, char* pcTaskName)
{
    printf("STACK OVERFLOW in task: %s\n", pcTaskName);
    configASSERT(false && "Stack overflow detected");
}
