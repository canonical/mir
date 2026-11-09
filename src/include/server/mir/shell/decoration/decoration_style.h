/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_SHELL_DECORATION_STYLE_H_
#define MIR_SHELL_DECORATION_STYLE_H_

#include <mir/geometry/displacement.h>
#include <mir/geometry/point.h>
#include <mir/geometry/size.h>
#include <mir_toolkit/common.h>

#include <endian.h>

#include <cstdint>

namespace mir::shell::decoration
{
using Pixel = uint32_t;

constexpr auto pack_pixel(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 0xFF) -> Pixel
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
    return ((Pixel)b <<  0) |
           ((Pixel)g <<  8) |
           ((Pixel)r << 16) |
           ((Pixel)a << 24);
#elif __BYTE_ORDER == __BIG_ENDIAN
    return ((Pixel)b << 24) |
           ((Pixel)g << 16) |
           ((Pixel)r <<  8) |
           ((Pixel)a <<  0);
#else
#error unsupported byte order
#endif
}

/// The red, green, blue and alpha channels of a \ref Pixel
struct Channels
{
    unsigned char r, g, b, a;
};

constexpr auto unpack_pixel(Pixel pixel) -> Channels
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
    return {
        (unsigned char)(pixel >> 16),
        (unsigned char)(pixel >>  8),
        (unsigned char)(pixel >>  0),
        (unsigned char)(pixel >> 24)};
#elif __BYTE_ORDER == __BIG_ENDIAN
    return {
        (unsigned char)(pixel >>  8),
        (unsigned char)(pixel >> 16),
        (unsigned char)(pixel >> 24),
        (unsigned char)(pixel >>  0)};
#else
#error unsupported byte order
#endif
}

enum class BorderType
{
    Full,       ///< Full titlebar and border (for restored windows)
    Titlebar,   ///< Titlebar only (for maximized windows)
    None,       ///< No decorations (for fullscreen windows or popups)
};

auto border_type_for(MirWindowType type, MirWindowState state) -> BorderType;

/// Colors used to draw the titlebar and borders of a window
struct Theme
{
    Pixel background_color;     ///< Color for background of the titlebar and borders
    Pixel text_color;           ///< Color the window title is drawn in
};

/// Colors used to draw a titlebar button
///
/// \note miral::DecorationStyle derives any of these that haven't been set
///       explicitly from the corresponding \ref Theme
struct ButtonTheme
{
    Pixel normal_color;         ///< Color of the background of the button area
    Pixel active_color;         ///< Color of the background when the button is active
    Pixel icon_color;           ///< Color of the icon
    Pixel unfocused_color;      ///< Color of the background while the window is unfocused
    Pixel unfocused_icon_color; ///< Color of the icon while the window is unfocused
};

/// The appearance of the decorations drawn by the default decoration strategy.
/// Defaults reproduce Mir's built-in look; a shell can override any subset.
struct DecorationStyle
{
    geometry::Height titlebar_height{24};           ///< Visible height of the top titlebar with the window's name and buttons
    geometry::Width side_border_width{6};           ///< Visible width of the side borders
    geometry::Height bottom_border_height{6};       ///< Visible height of the bottom border
    geometry::Size resize_corner_input_size{16, 16};///< The size of the input area of a resizable corner
                                                    ///< (does not effect surface input area, only where in the
                                                    ///< surface is considered a resize corner)
    geometry::Width button_width{24};               ///< The width of window control buttons
    geometry::Width padding_between_buttons{6};     ///< The gap between titlebar buttons
    geometry::Height title_font_height{14};         ///< Height of the text used to render the window title
    geometry::Point title_font_top_left{8, 2};      ///< Where to render the window title
    geometry::Displacement icon_padding{5, 5};      ///< Padding inside buttons around icons
    geometry::Width icon_line_width{1};             ///< Width for lines in button icons
    geometry::Width corner_radius{0};               ///< Radius of the rounded outer corners of the decorations
                                                    ///< (zero for square corners)

    Theme focused{pack_pixel(0x32, 0x32, 0x32), pack_pixel(0xFF, 0xFF, 0xFF)};      ///< Colors used while focused
    Theme unfocused{pack_pixel(0x80, 0x80, 0x80), pack_pixel(0xA0, 0xA0, 0xA0)};    ///< Colors used while unfocused

    ButtonTheme button{
        pack_pixel(0x65, 0x65, 0x65),
        pack_pixel(0x98, 0x98, 0x98),
        pack_pixel(0xFF, 0xFF, 0xFF),
        pack_pixel(0x88, 0x88, 0x88),
        pack_pixel(0xA0, 0xA0, 0xA0)};              ///< Colors used for the minimize and maximize buttons
    ButtonTheme close_button{
        pack_pixel(0xA0, 0x20, 0x20),
        pack_pixel(0xC0, 0x60, 0x60),
        pack_pixel(0xFF, 0xFF, 0xFF),
        pack_pixel(0x80, 0x50, 0x50),
        pack_pixel(0xA0, 0xA0, 0xA0)};              ///< Colors used for the close button

    MirPixelFormat buffer_format{mir_pixel_format_argb_8888};
};

}

#endif //MIR_SHELL_DECORATION_STYLE_H_
