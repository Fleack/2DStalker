#include "Style.hpp"

#include <utility>

namespace s2d::client::ui::style
{
tgui::Color const background{10, 14, 23};
tgui::Color const surface{17, 24, 39};
tgui::Color const surface2{24, 32, 48};
tgui::Color const border{43, 54, 76};
tgui::Color const text{235, 241, 250};
tgui::Color const muted{148, 163, 184};
tgui::Color const accent{96, 165, 250};
tgui::Color const accentHover{125, 184, 255};
tgui::Color const accentDown{59, 130, 246};
tgui::Color const ok{74, 222, 128};
tgui::Color const warn{251, 191, 36};
tgui::Color const err{248, 113, 113};

namespace
{
tgui::Color const dangerBackground{88, 28, 28};
tgui::Color const dangerHover{127, 29, 29};
tgui::Color const dangerDown{153, 27, 27};
} // namespace

tgui::Label::Ptr makeLabel(
    std::string labelText,
    unsigned int textSize,
    tgui::Color color)
{
    auto label = tgui::Label::create(std::move(labelText));
    label->setTextSize(textSize);
    label->getRenderer()->setTextColor(color);
    return label;
}

void stylePanel(
    tgui::Panel::Ptr const& panel,
    tgui::Color panelBackground,
    tgui::Color panelBorder,
    float radius)
{
    auto* renderer = panel->getRenderer();
    renderer->setBackgroundColor(panelBackground);
    renderer->setBorderColor(panelBorder);
    renderer->setBorders(tgui::Borders{1.F});
    renderer->setRoundedBorderRadius(radius);
}

tgui::Button::Ptr makeButton(
    std::string buttonText,
    float y,
    ButtonKind kind)
{
    auto button = tgui::Button::create(std::move(buttonText));
    button->setPosition(24.F, y);
    button->setSize(212.F, 44.F);
    button->setTextSize(15);

    auto* renderer = button->getRenderer();
    renderer->setBorders(tgui::Borders{0.F});
    renderer->setRoundedBorderRadius(10.F);
    renderer->setTextColor(text);

    switch (kind)
    {
    case ButtonKind::Primary:
        renderer->setBackgroundColor(accentDown);
        renderer->setBackgroundColorHover(accent);
        renderer->setBackgroundColorDown(accentHover);
        break;
    case ButtonKind::Secondary:
        renderer->setBackgroundColor(surface2);
        renderer->setBackgroundColorHover(tgui::Color{35, 45, 66});
        renderer->setBackgroundColorDown(tgui::Color{47, 60, 86});
        break;
    case ButtonKind::Danger:
        renderer->setBackgroundColor(dangerBackground);
        renderer->setBackgroundColorHover(dangerHover);
        renderer->setBackgroundColorDown(dangerDown);
        break;
    }
    return button;
}
} // namespace s2d::client::ui::style
