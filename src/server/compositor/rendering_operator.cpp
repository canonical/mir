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

namespace mc=mir::compositor;

mc::RenderingOperator::RenderingOperator(
    Renderer& renderer,
    std::function<void(std::shared_ptr<void> const&)> save_resource,
    unsigned long frameno) :
    renderer(renderer),
    save_resource(save_resource),
    frameno(frameno)
{
}

void mc::RenderingOperator::operator()(CompositingCriteria const& info, BufferStream& stream)
{
    auto compositor_buffer = stream.lock_compositor_buffer(frameno);
    renderer.render(info, *compositor_buffer);
    save_resource(compositor_buffer);
}
