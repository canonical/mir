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
#include <miral/live_config.h>

#include <mir/log.h>
#include <mir/server.h>
#include <mir/shell/decoration/decoration_style.h>

#include <charconv>
#include <mutex>

namespace msd = mir::shell::decoration;

namespace
{
using Element = miral::DecorationStyle::Element;
using State = miral::DecorationStyle::State;
using Metric = miral::DecorationStyle::Metric;

/// The color of \p element in \p state, or nullptr if the combination isn't drawn
auto color_of(msd::DecorationStyle& style, Element element, State state) -> msd::Pixel*
{
    switch (element)
    {
    case Element::background:
        switch (state)
        {
        case State::focused:    return &style.focused.background_color;
        case State::unfocused:  return &style.unfocused.background_color;
        default:                return nullptr;
        }

    case Element::title:
        switch (state)
        {
        case State::focused:    return &style.focused.text_color;
        case State::unfocused:  return &style.unfocused.text_color;
        default:                return nullptr;
        }

    case Element::button:
        switch (state)
        {
        case State::normal:     return &style.button.normal_color;
        case State::hovered:    return &style.button.active_color;
        default:                return nullptr;
        }

    case Element::close_button:
        switch (state)
        {
        case State::normal:     return &style.close_button.normal_color;
        case State::hovered:    return &style.close_button.active_color;
        default:                return nullptr;
        }

    // Icons are drawn the same way whatever the state
    case Element::button_icon:          return &style.button.icon_color;
    case Element::close_button_icon:    return &style.close_button.icon_color;

    default:                            return nullptr;
    }
}

auto metric_of(msd::DecorationStyle const& style, Metric metric) -> std::optional<int>
{
    switch (metric)
    {
    case Metric::titlebar_height:           return style.titlebar_height.as_value();
    case Metric::side_border_width:         return style.side_border_width.as_value();
    case Metric::bottom_border_height:      return style.bottom_border_height.as_value();
    case Metric::button_width:              return style.button_width.as_value();
    case Metric::padding_between_buttons:   return style.padding_between_buttons.as_value();
    case Metric::title_font_height:         return style.title_font_height.as_value();
    case Metric::resize_corner_input_width: return style.resize_corner_input_size.width.as_value();
    case Metric::resize_corner_input_height:return style.resize_corner_input_size.height.as_value();
    case Metric::corner_radius:             return style.corner_radius.as_value();
    default:                                return std::nullopt;
    }
}

auto set_metric_of(msd::DecorationStyle& style, Metric metric, int pixels) -> bool
{
    switch (metric)
    {
    case Metric::titlebar_height:
        style.titlebar_height = mir::geometry::Height{pixels};
        return true;

    case Metric::side_border_width:
        style.side_border_width = mir::geometry::Width{pixels};
        return true;

    case Metric::bottom_border_height:
        style.bottom_border_height = mir::geometry::Height{pixels};
        return true;

    case Metric::button_width:
        style.button_width = mir::geometry::Width{pixels};
        return true;

    case Metric::padding_between_buttons:
        style.padding_between_buttons = mir::geometry::Width{pixels};
        return true;

    case Metric::title_font_height:
        style.title_font_height = mir::geometry::Height{pixels};
        return true;

    case Metric::resize_corner_input_width:
        style.resize_corner_input_size.width = mir::geometry::Width{pixels};
        return true;

    case Metric::resize_corner_input_height:
        style.resize_corner_input_size.height = mir::geometry::Height{pixels};
        return true;

    case Metric::corner_radius:
        style.corner_radius = mir::geometry::Width{pixels};
        return true;

    default:
        return false;
    }
}

/// Parses "#rrggbb" or "#rrggbbaa" (the leading '#' is optional)
auto parse_color(std::string_view text) -> std::optional<msd::Pixel>
{
    if (text.starts_with('#'))
        text.remove_prefix(1);

    if (text.size() != 6 && text.size() != 8)
        return std::nullopt;

    uint32_t channels[4] = {0, 0, 0, 0xFF};
    uint32_t* channel = channels;

    for (auto i = 0u; i != text.size(); i += 2, ++channel)
    {
        auto const begin = text.data() + i;
        if (auto const [end, ec] = std::from_chars(begin, begin + 2, *channel, 16);
            ec != std::errc{} || end != begin + 2)
            return std::nullopt;
    }

    return msd::pack_pixel(channels[0], channels[1], channels[2], channels[3]);
}

struct ColorKey
{
    char const* element_name;
    char const* state_name;     ///< nullptr if the element is drawn the same in all states
    Element element;
    State state;
};

ColorKey constexpr color_keys[] = {
    {"background",          "focused",      Element::background,        State::focused},
    {"background",          "unfocused",    Element::background,        State::unfocused},
    {"title",               "focused",      Element::title,             State::focused},
    {"title",               "unfocused",    Element::title,             State::unfocused},
    {"button",              "normal",       Element::button,            State::normal},
    {"button",              "hovered",      Element::button,            State::hovered},
    {"button_icon",         nullptr,        Element::button_icon,       State::normal},
    {"close_button",        "normal",       Element::close_button,      State::normal},
    {"close_button",        "hovered",      Element::close_button,      State::hovered},
    {"close_button_icon",   nullptr,        Element::close_button_icon, State::normal},
};

struct MetricKey
{
    char const* name;
    char const* description;
    Metric metric;
};

MetricKey constexpr metric_keys[] = {
    {"titlebar_height",             "Visible height of the titlebar",           Metric::titlebar_height},
    {"side_border_width",           "Visible width of the side borders",        Metric::side_border_width},
    {"bottom_border_height",        "Visible height of the bottom border",      Metric::bottom_border_height},
    {"button_width",                "Width of the titlebar buttons",            Metric::button_width},
    {"padding_between_buttons",     "Gap between the titlebar buttons",         Metric::padding_between_buttons},
    {"title_font_height",           "Height of the window title text",          Metric::title_font_height},
    {"resize_corner_input_width",   "Width of the resize handle in a corner",   Metric::resize_corner_input_width},
    {"resize_corner_input_height",  "Height of the resize handle in a corner",  Metric::resize_corner_input_height},
    {"corner_radius",               "Radius of the rounded outer corners",      Metric::corner_radius},
};
}

struct miral::DecorationStyle::Self
{
    std::mutex mutex;
    msd::DecorationStyle style;

    /// Only set while the server is running
    mir::Server* server = nullptr;

    void apply()
    {
        std::lock_guard lock{mutex};
        apply_locked();
    }

    void apply_locked()
    {
        if (server)
            server->set_the_decoration_style(std::make_shared<msd::DecorationStyle const>(style));
    }
};

miral::DecorationStyle::DecorationStyle() :
    self{std::make_shared<Self>()}
{
}

miral::DecorationStyle::DecorationStyle(live_config::Store& config_store) :
    DecorationStyle{}
{
    for (auto const& [name, description, metric] : metric_keys)
    {
        config_store.add_int_attribute(
            {"ssd", name},
            description,
            [self=self, metric=metric](live_config::Key const& key, std::optional<int> val)
            {
                std::lock_guard lock{self->mutex};

                if (val && *val < 0)
                {
                    mir::log_warning("Config key '%s' cannot be negative", key.to_string().c_str());
                    return;
                }

                // Reverting to the default is the point of an unset value
                set_metric_of(self->style, metric, val.value_or(metric_of(msd::DecorationStyle{}, metric).value()));
            });
    }

    for (auto const& [element_name, state_name, element, state] : color_keys)
    {
        auto const key = state_name
            ? live_config::Key{"ssd", "color", element_name, state_name}
            : live_config::Key{"ssd", "color", element_name};

        config_store.add_string_attribute(
            key,
            "Color as '#rrggbb' or '#rrggbbaa'",
            [self=self, element=element, state=state](live_config::Key const& key, std::optional<std::string_view> val)
            {
                std::lock_guard lock{self->mutex};

                msd::DecorationStyle defaults;
                auto const color = val
                    ? parse_color(*val)
                    : std::optional{*color_of(defaults, element, state)};

                if (!color)
                {
                    mir::log_warning(
                        "Config key '%s' is not a color of the form '#rrggbb' or '#rrggbbaa'",
                        key.to_string().c_str());
                    return;
                }

                *color_of(self->style, element, state) = *color;
            });
    }

    config_store.on_done([self=self] { self->apply(); });
}

miral::DecorationStyle::DecorationStyle(DecorationStyle const&) = default;

auto miral::DecorationStyle::operator=(DecorationStyle const&) -> DecorationStyle& = default;

miral::DecorationStyle::~DecorationStyle() = default;

void miral::DecorationStyle::set_color(
    Element element, State state, uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha)
{
    std::lock_guard lock{self->mutex};

    if (auto const color = color_of(self->style, element, state))
    {
        *color = msd::pack_pixel(red, green, blue, alpha);
        self->apply_locked();
    }
    else
    {
        mir::log_warning(
            "Ignoring decoration color for unsupported element %d in state %d",
            static_cast<int>(element), static_cast<int>(state));
    }
}

void miral::DecorationStyle::set_metric(Metric metric, int pixels)
{
    std::lock_guard lock{self->mutex};

    if (pixels < 0)
    {
        mir::log_warning("Ignoring negative decoration metric %d", static_cast<int>(metric));
        return;
    }

    if (set_metric_of(self->style, metric, pixels))
        self->apply_locked();
    else
        mir::log_warning("Ignoring unsupported decoration metric %d", static_cast<int>(metric));
}

void miral::DecorationStyle::operator()(mir::Server& server) const
{
    server.add_pre_init_callback(
        [self=self, &server]
        {
            std::lock_guard lock{self->mutex};
            self->server = &server;
            self->apply_locked();
        });

    server.add_stop_callback(
        [self=self]
        {
            std::lock_guard lock{self->mutex};
            self->server = nullptr;
        });
}
