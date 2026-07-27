#include "EventLogPanel.hpp"

#include "Style.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <utility>

namespace
{
std::string currentTime()
{
    auto const now = std::chrono::system_clock::now();
    auto const time = std::chrono::system_clock::to_time_t(now);
    std::tm localTime{};
    localtime_s(&localTime, &time);

    std::ostringstream output;
    output << std::put_time(&localTime, "%H:%M:%S");
    return output.str();
}
} // namespace

namespace s2d::client
{
EventLogPanel::EventLogPanel(tgui::Panel& root)
    : m_logView{tgui::TextArea::create()}
{
    auto logTitle = ui::style::makeLabel("Event log", 18, ui::style::text);
    logTitle->setPosition(324.F, 244.F);
    auto logHint =
        ui::style::makeLabel("Recent client activity", 13, ui::style::muted);
    logHint->setPosition(324.F, 270.F);

    m_logView->setPosition(324.F, 304.F);
    m_logView->setSize(644.F, 344.F);
    m_logView->setTextSize(14);
    m_logView->setReadOnly(true);

    auto* renderer = m_logView->getRenderer();
    renderer->setBackgroundColor(ui::style::surface);
    renderer->setTextColor(ui::style::text);
    renderer->setDefaultTextColor(ui::style::muted);
    renderer->setBorderColor(ui::style::border);
    renderer->setBorders(tgui::Borders{1.F});
    renderer->setPadding(tgui::Padding{16.F, 14.F, 16.F, 14.F});
    renderer->setRoundedBorderRadius(18.F);
    renderer->setCaretColor(tgui::Color::Transparent);

    root.add(logTitle);
    root.add(logHint);
    root.add(m_logView);
}

void EventLogPanel::add(std::string line)
{
    if (m_lines.size() >= maxLines)
    {
        m_lines.pop_front();
    }

    std::string entry;
    entry.reserve(line.size() + 16);
    entry += '[';
    entry += currentTime();
    entry += "] ";
    entry += line;
    m_lines.push_back(std::move(entry));
    refresh();
}

void EventLogPanel::refresh()
{
    std::string text;
    for (auto const& line : m_lines)
    {
        text += line;
        text += '\n';
    }

    m_logView->setText(text);
    auto scrollbar = m_logView->getVerticalScrollbar();
    scrollbar->setValue(scrollbar->getMaxValue());
}
} // namespace s2d::client
