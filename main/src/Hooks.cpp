#include "Hooks.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <span>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/FreeRTOSConfig.h"
#include "freertos/task.h"

namespace
{
const char* TAG = "CPU_MON";
}

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

/**
 * @brief Prints CPU usage and task details using C++20 spans and ESP Log API.
 */
void monitor_cpu_usage(std::span<TaskStatus_t> tasks, std::uint32_t total_runtime)
{
    if (total_runtime == 0 || tasks.empty())
    {
        return;
    }

    ESP_LOGI(TAG, "%-16s | %-10s | %-10s", "Task Name", "Run Time", "CPU %");
    ESP_LOGI(TAG, "--------------------------------------------------");

    std::uint32_t idle_runtime = 0;

    for (const auto& ts : tasks)
    {
        // Calculate percentage since boot
        float percentage = (static_cast<float>(ts.ulRunTimeCounter) / total_runtime) * 100.0f;

        // Using ESP_LOGI for each task entry
        ESP_LOGI(TAG,
                 "%-16s | %-10lu | %.2f%%",
                 ts.pcTaskName,
                 static_cast<unsigned long>(ts.ulRunTimeCounter),
                 percentage);

        if (std::strcmp(ts.pcTaskName, "IDLE") == 0)
        {
            idle_runtime = ts.ulRunTimeCounter;
        }
    }

    // Calculate Total Busy CPU Usage
    float total_cpu_load = 100.0f - ((static_cast<float>(idle_runtime) / total_runtime) * 100.0f);
    ESP_LOGI(TAG, "--------------------------------------------------");
    ESP_LOGI(TAG, "TOTAL CPU LOAD: %.2f%%", total_cpu_load);
}

extern "C" void stack_monitor_task(void* arg)
{
    (void)arg;

#if (configUSE_TRACE_FACILITY == 1 && configGENERATE_RUN_TIME_STATS == 1)
    while (true)
    {
        UBaseType_t task_count = uxTaskGetNumberOfTasks();
        auto* task_array =
            static_cast<TaskStatus_t*>(pvPortMalloc(task_count * sizeof(TaskStatus_t)));

        if (task_array != nullptr)
        {
            std::uint32_t total_runtime = 0;
            UBaseType_t valid_count = uxTaskGetSystemState(task_array, task_count, &total_runtime);

            // C++20 span creation
            std::span<TaskStatus_t> task_span(task_array, valid_count);

            // 1. CPU Monitor function
            monitor_cpu_usage(task_span, total_runtime);

            // 2. Stack High Water Mark logic (using ESP_LOGI)
            ESP_LOGI("STACK", "---- Stack High Water Marks ----");
            for (const auto& ts : task_span)
            {
                std::uint32_t free_bytes =
                    static_cast<std::uint32_t>(ts.usStackHighWaterMark) * sizeof(StackType_t);
                ESP_LOGI(
                    "STACK", "%-16s: %lu bytes free", ts.pcTaskName, (unsigned long)free_bytes);
            }

            vPortFree(task_array);
        }
        else
        {
            ESP_LOGE(TAG, "Memory allocation failed");
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
#else
    ESP_LOGW(TAG,
             "Enable configUSE_TRACE_FACILITY and configGENERATE_RUN_TIME_STATS in menuconfig");
    vTaskDelete(nullptr);
#endif
}