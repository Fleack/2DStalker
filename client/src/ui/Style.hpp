#pragma once

#include <string>

#include <TGUI/Color.hpp>
#include <TGUI/Widgets/Button.hpp>
#include <TGUI/Widgets/Label.hpp>
#include <TGUI/Widgets/Panel.hpp>

namespace s2d::client::ui::style
{
extern tgui::Color const background;
extern tgui::Color const surface;
extern tgui::Color const surface2;
extern tgui::Color const border;
extern tgui::Color const text;
extern tgui::Color const muted;
extern tgui::Color const accent;
extern tgui::Color const accentHover;
extern tgui::Color const accentDown;
extern tgui::Color const ok;
extern tgui::Color const warn;
extern tgui::Color const err;

enum class ButtonKind
{
    Primary,
    Secondary,
    Danger,
};

[[nodiscard]] tgui::Label::Ptr makeLabel(
    std::string labelText,
    unsigned int textSize,
    tgui::Color color);

void stylePanel(
    tgui::Panel::Ptr const& panel,
    tgui::Color panelBackground,
    tgui::Color panelBorder,
    float radius = 14.F);

[[nodiscard]] tgui::Button::Ptr makeButton(
    std::string buttonText,
    float y,
    ButtonKind kind = ButtonKind::Secondary);
} // namespace s2d::client::ui::style
