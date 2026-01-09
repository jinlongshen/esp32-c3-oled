#ifndef COMPONENT_LVGL_DRIVER_LVGL_TASKS_H
#define COMPONENT_LVGL_DRIVER_LVGL_TASKS_H

namespace muc::lvgl_driver
{

void lvgl_tick_task(void* arg);
void lvgl_handler_task(void* arg);

} // namespace muc::lvgl_driver

#endif // COMPONENT_LVGL_DRIVER_LVGL_TASKS_H
