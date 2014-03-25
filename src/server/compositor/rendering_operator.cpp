/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "rendering_operator.h"
#include "mir/compositor/buffer_stream.h"
#include "mir/graphics/renderable.h"

namespace mc=mir::compositor;

mc::RenderingOperator::RenderingOperator(
    Renderer& renderer)
    : renderer(renderer),
      uncomposited_buffers_{false}
{
}

void mc::RenderingOperator::operator()(graphics::Renderable const& renderable)
{
    uncomposited_buffers_ |= renderable.buffers_ready_for_compositor() > 1;
    auto compositor_buffer = renderable.buffer(&renderer);
    // preserves buffers used in rendering until after post_update()
    saved_resources.push_back(compositor_buffer);
    renderer.render(renderable, *compositor_buffer);
}

bool mc::RenderingOperator::uncomposited_buffers() const
{
    return uncomposited_buffers_;
}

bool mc::RenderingOperator::anything_was_rendered() const
{
    return !saved_resources.empty();
}
