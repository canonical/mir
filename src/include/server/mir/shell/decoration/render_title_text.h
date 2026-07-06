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

#ifndef MIR_SHELL_DECORATION_RENDER_TITLE_TEXT_H_
#define MIR_SHELL_DECORATION_RENDER_TITLE_TEXT_H_

#include <mir/geometry/point.h>
#include <mir/geometry/size.h>

#include <cstdint>
#include <string>

namespace mir::shell::decoration
{

using Pixel = uint32_t;

/// FreeType title rendering helper for the built-in decoration renderer and
/// MirAL's default DecorationStrategy::render_title_text(). Exported from
/// libmirserver for MirAL linking only; unstable — not a supported public API.
void render_title_text(
    Pixel* buf,
    geometry::Size buf_size,
    std::string const& text,
    geometry::Point top_left,
    geometry::Height height_pixels,
    Pixel color);

}

#endif // MIR_SHELL_DECORATION_RENDER_TITLE_TEXT_H_
