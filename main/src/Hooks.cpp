#include "Hooks.h"

#include <cstdint>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/FreeRTOSConfig.h"
#include "freertos/task.h"

extern "C" void vApplicationStackOverflowHook(TaskHandle_t xTask, char* pcTaskName)
{
    // Stack overflow is a fatal error: log at ERROR level
    ESP_LOGE("FREERTOS", "STACK OVERFLOW in task: %s (handle=%p)", pcTaskName, xTask);
    configASSERT(false && "Stack overflow detected");
}

extern "C" void vApplicationIdleHook(void)
{
    static std::uint32_t idle_counter = 0;
    idle_counter++;

    // Simple heartbeat: log occasionally to verify CPU0 is actually idling
    if ((idle_counter % 1000000U) == 0U)
    {
        ESP_LOGI("IDLE", "CPU0 idle counter = %u", idle_counter);
    }
}

extern "C" void stack_monitor_task(void* arg)
{
    (void)arg;

#if (configUSE_TRACE_FACILITY == 1)
    // This task prints stack high water marks for all tasks periodically
    while (true)
    {
        ESP_LOGI("STACK", "---- Stack High Water Marks ----");

        UBaseType_t task_count = uxTaskGetNumberOfTasks();
        TaskStatus_t* task_status_array =
            static_cast<TaskStatus_t*>(pvPortMalloc(task_count * sizeof(TaskStatus_t)));

        if (task_status_array != nullptr)
        {
            UBaseType_t valid_count = uxTaskGetSystemState(task_status_array, task_count, nullptr);

            for (UBaseType_t i = 0; i < valid_count; i++)
            {
                const TaskStatus_t& ts = task_status_array[i];
                // usStackHighWaterMark is in words (StackType_t), convert to bytes
                uint32_t free_bytes =
                    static_cast<uint32_t>(ts.usStackHighWaterMark) * sizeof(StackType_t);

                ESP_LOGI("STACK", "%s: %u bytes free", ts.pcTaskName, free_bytes);
            }

            vPortFree(task_status_array);
        }
        else
        {
            ESP_LOGE("STACK", "Failed to allocate task status array");
        }

        vTaskDelay(pdMS_TO_TICKS(5000)); // print every 5 seconds
    }
#else
    // If trace facility is disabled, this task cannot report stack usage
    ESP_LOGW("STACK", "configUSE_TRACE_FACILITY is 0; stack_monitor_task is inactive");
    vTaskDelete(nullptr);
#endif
}
