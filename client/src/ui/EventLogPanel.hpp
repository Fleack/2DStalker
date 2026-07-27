#pragma once

#include <deque>
#include <string>

#include <TGUI/Widgets/Panel.hpp>
#include <TGUI/Widgets/TextArea.hpp>

namespace s2d::client
{
class EventLogPanel
{
public:
    explicit EventLogPanel(tgui::Panel& root);

    void add(std::string line);

private:
    static constexpr std::size_t maxLines = 80;

    void refresh();

    tgui::TextArea::Ptr m_logView;
    std::deque<std::string> m_lines;
};
} // namespace s2d::client
