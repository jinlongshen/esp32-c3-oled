#ifndef COMPONENTS_UI_INC_UI_API_H
#define COMPONENTS_UI_INC_UI_API_H

#include <string_view>

#include "ui_queue.h"

namespace muc::ui
{

class UiApi
{
  public:
    explicit UiApi(muc::ui::UiQueue& queue);

    void set_text(std::string_view text);
    void set_status(std::string_view status);
    void show_provision_qr(std::string_view payload);

  private:
    muc::ui::UiQueue& m_queue;
};

} // namespace muc::ui

#endif // COMPONENTS_UI_INC_UI_API_H