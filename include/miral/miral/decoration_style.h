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

#ifndef MIRAL_DECORATION_STYLE_H
#define MIRAL_DECORATION_STYLE_H

#include <cstdint>
#include <memory>

namespace mir { class Server; }

namespace miral
{
namespace live_config { class Store; }

/// Customizes the appearance of the decorations Mir draws for server side
/// decorated windows.
///
/// Anything left unset keeps Mir's default appearance. Changes made after the
/// server has started are applied to the windows that are already decorated.
///
/// Copies share the same settings.
///
/// \note Unrecognized Element, State and Metric values are ignored, so that
///       new ones can be introduced without breaking ABI.
/// \remark Since MirAL 6.2
class DecorationStyle
{
public:
    DecorationStyle();

    /// Constructor registering with a configuration store
    ///
    /// Available options:
    ///     - {ssd,titlebar_height}: Visible height of the titlebar
    ///     - {ssd,side_border_width}: Visible width of the side borders
    ///     - {ssd,bottom_border_height}: Visible height of the bottom border
    ///     - {ssd,button_width}: Width of the titlebar buttons
    ///     - {ssd,padding_between_buttons}: Gap between the titlebar buttons
    ///     - {ssd,title_font_height}: Height of the window title text
    ///     - {ssd,resize_corner_input_width}: Width of the resize handle in a corner
    ///     - {ssd,resize_corner_input_height}: Height of the resize handle in a corner
    ///     - {ssd,corner_radius}: Radius of the rounded outer corners (zero for square corners)
    ///     - {ssd,color,background,focused}
    ///     - {ssd,color,background,unfocused}
    ///     - {ssd,color,title,focused}
    ///     - {ssd,color,title,unfocused}
    ///     - {ssd,color,button,normal}
    ///     - {ssd,color,button,hovered}
    ///     - {ssd,color,button_icon}
    ///     - {ssd,color,close_button,normal}
    ///     - {ssd,color,close_button,hovered}
    ///     - {ssd,color,close_button_icon}
    ///
    /// Colors are written as '#rrggbb' or '#rrggbbaa'. Options that are unset
    /// revert to Mir's defaults.
    explicit DecorationStyle(live_config::Store& config_store);

    DecorationStyle(DecorationStyle const&);
    auto operator=(DecorationStyle const&) -> DecorationStyle&;
    ~DecorationStyle();

    /// The parts of a decoration that can be colored
    enum class Element : uint32_t
    {
        background,        ///< Background of the titlebar and the borders
        title,             ///< The window title text
        button,            ///< Background of the minimize and maximize buttons
        button_icon,       ///< Icons of the minimize and maximize buttons
        close_button,      ///< Background of the close button
        close_button_icon, ///< Icon of the close button
    };

    /// The state an element is drawn in
    enum class State : uint32_t
    {
        focused,   ///< The window has keyboard focus
        unfocused, ///< The window does not have keyboard focus
        normal,    ///< The pointer is not over the element (buttons only)
        hovered,   ///< The pointer is over the element (buttons only)
    };

    /// Lengths of the parts of a decoration, in logical pixels
    enum class Metric : uint32_t
    {
        titlebar_height,            ///< Visible height of the titlebar
        side_border_width,          ///< Visible width of the side borders
        bottom_border_height,       ///< Visible height of the bottom border
        button_width,               ///< Width of the titlebar buttons
        padding_between_buttons,    ///< Gap between the titlebar buttons
        title_font_height,          ///< Height of the window title text
        resize_corner_input_width,  ///< Width of the resize handle in a corner
        resize_corner_input_height, ///< Height of the resize handle in a corner
        corner_radius,              ///< Radius of the rounded outer corners (zero for square corners)
    };

    /// Set the color used to draw \p element while in \p state.
    ///
    /// \ref Element::background and \ref Element::title are drawn in either
    /// \ref State::focused or \ref State::unfocused; the button elements are
    /// drawn in either \ref State::normal or \ref State::hovered.
    ///
    /// Takes effect immediately, or when the server starts.
    void set_color(Element element, State state, uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 0xFF);

    /// Set \p metric to \p pixels logical pixels
    ///
    /// Takes effect immediately, or when the server starts.
    void set_metric(Metric metric, int pixels);

    void operator()(mir::Server& server) const;

private:
    struct Self;
    std::shared_ptr<Self> self;
};
}

#endif // MIRAL_DECORATION_STYLE_H
