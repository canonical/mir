/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_EXAMPLES_AS_RENDER_TARGET_H_
#define MIR_EXAMPLES_AS_RENDER_TARGET_H_

#include "mir/graphics/display_buffer.h"
#include "mir/renderer/gl/render_target.h"

#include <stdexcept>

namespace mir
{
namespace examples
{

inline auto as_render_target(graphics::DisplayBuffer& display_buffer)
{
    auto const render_target =
        dynamic_cast<renderer::gl::RenderTarget*>(display_buffer.native_display_buffer());
    if (!render_target)
        throw std::logic_error{"DisplayBuffer doesn't support GL rendering"};
    return render_target;
}

}
}

#endif
