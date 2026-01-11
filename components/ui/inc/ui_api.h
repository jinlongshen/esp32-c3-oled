#ifndef COMPONENTS_UI_INC_UI_API_H
#define COMPONENTS_UI_INC_UI_API_H

#include "ui_queue.h"
#include <string_view>

namespace muc::ui
{

class UiApi
{
  public:
    explicit UiApi(UiQueue& queue);

    void set_text(std::string_view text);
    void set_status(std::string_view text);
    void set_cpu_usage(int percent);
    void clear_screen();

  private:
    UiQueue& m_queue;
};

} // namespace muc::ui

#endif // COMPONENTS_UI_INC_UI_API_H
