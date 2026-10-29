/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <miral/decoration_style.h>

#include <mir/log.h>
#include <mir/server.h>
#include <mir/shell/decoration/decoration_style.h>

namespace msd = mir::shell::decoration;

struct miral::DecorationStyle::Self
{
    msd::DecorationStyle style;
};

miral::DecorationStyle::DecorationStyle() :
    self{std::make_shared<Self>()}
{
}

miral::DecorationStyle::DecorationStyle(DecorationStyle const& that) :
    self{std::make_shared<Self>(*that.self)}
{
}

auto miral::DecorationStyle::operator=(DecorationStyle const& that) -> DecorationStyle&
{
    self = std::make_shared<Self>(*that.self);
    return *this;
}

miral::DecorationStyle::~DecorationStyle() = default;

void miral::DecorationStyle::set_color(
    Element element, State state, uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha)
{
    auto const color = msd::pack_pixel(red, green, blue, alpha);
    auto& style = self->style;

    switch (element)
    {
    case Element::background:
        switch (state)
        {
        case State::focused:    style.focused.background_color = color;     return;
        case State::unfocused:  style.unfocused.background_color = color;   return;
        default:;
        }
        break;

    case Element::title:
        switch (state)
        {
        case State::focused:    style.focused.text_color = color;   return;
        case State::unfocused:  style.unfocused.text_color = color; return;
        default:;
        }
        break;

    case Element::button:
        switch (state)
        {
        case State::normal:     style.button.normal_color = color;  return;
        case State::hovered:    style.button.active_color = color;  return;
        default:;
        }
        break;

    case Element::close_button:
        switch (state)
        {
        case State::normal:     style.close_button.normal_color = color;    return;
        case State::hovered:    style.close_button.active_color = color;    return;
        default:;
        }
        break;

    case Element::button_icon:
        style.button.icon_color = color;
        return;

    case Element::close_button_icon:
        style.close_button.icon_color = color;
        return;
    }

    mir::log_warning(
        "Ignoring decoration color for unsupported element %d in state %d",
        static_cast<int>(element), static_cast<int>(state));
}

void miral::DecorationStyle::set_metric(Metric metric, int pixels)
{
    auto& style = self->style;

    switch (metric)
    {
    case Metric::titlebar_height:
        style.titlebar_height = mir::geometry::Height{pixels};
        return;

    case Metric::side_border_width:
        style.side_border_width = mir::geometry::Width{pixels};
        return;

    case Metric::bottom_border_height:
        style.bottom_border_height = mir::geometry::Height{pixels};
        return;

    case Metric::button_width:
        style.button_width = mir::geometry::Width{pixels};
        return;

    case Metric::padding_between_buttons:
        style.padding_between_buttons = mir::geometry::Width{pixels};
        return;

    case Metric::title_font_height:
        style.title_font_height = mir::geometry::Height{pixels};
        return;

    case Metric::resize_corner_input_width:
        style.resize_corner_input_size.width = mir::geometry::Width{pixels};
        return;

    case Metric::resize_corner_input_height:
        style.resize_corner_input_size.height = mir::geometry::Height{pixels};
        return;
    }

    mir::log_warning("Ignoring unsupported decoration metric %d", static_cast<int>(metric));
}

void miral::DecorationStyle::operator()(mir::Server& server) const
{
    server.add_pre_init_callback(
        [self = self, &server]
        {
            server.set_the_decoration_style(std::make_shared<msd::DecorationStyle const>(self->style));
        });
}
