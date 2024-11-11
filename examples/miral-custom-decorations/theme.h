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

#ifndef MIRAL_CUSTOM_DECORATIONS_THEME_H
#define MIRAL_CUSTOM_DECORATIONS_THEME_H

#include "mircore/mir/geometry/forward.h"

#include "input.h"

#include <cstdint>
#include <functional>
#include <map>

using Pixel = uint32_t;

/// A visual theme for a decoration
/// Focused and unfocused windows use a different theme
struct Theme
{
    Pixel const background_color; ///< Color for background of the titlebar and borders
    Pixel const text_color;       ///< Color the window title is drawn in
};

// Info needed to render a button icon
struct Icon
{
    Pixel const normal_color; ///< Normal of the background of the button area
    Pixel const active_color; ///< Color of the background when button is active
    Pixel const icon_color;   ///< Color of the icon

    using DrawingFunction = std::function<void(
        Pixel* const data,
        mir::geometry::Size buf_size,
        mir::geometry::Rectangle box,
        mir::geometry::Width line_width,
        Pixel color)>; ///< Draws button's icon to the given buffer
};

using ButtonTheme = std::map<ButtonFunction, Icon const>;

static inline constexpr auto color(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 0xFF)
    -> uint32_t
{
    return ((uint32_t)b << 0) | ((uint32_t)g << 8) | ((uint32_t)r << 16) | ((uint32_t)a << 24);
}

#endif
