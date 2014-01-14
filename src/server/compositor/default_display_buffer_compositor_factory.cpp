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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "default_display_buffer_compositor_factory.h"
#include "renderer_factory.h"
#include "renderer.h"
#include "mir/graphics/display_buffer.h"

#include "default_display_buffer_compositor.h"

namespace mc = mir::compositor;
namespace mg = mir::graphics;

mc::DefaultDisplayBufferCompositorFactory::DefaultDisplayBufferCompositorFactory(
    std::shared_ptr<mc::Scene> const& scene,
    std::shared_ptr<mc::RendererFactory> const& renderer_factory,
    std::shared_ptr<mc::CompositorReport> const& report)
    : scene{scene},
      renderer_factory{renderer_factory},
      report{report}
{
}

std::unique_ptr<mc::DisplayBufferCompositor>
mc::DefaultDisplayBufferCompositorFactory::create_compositor_for(
    graphics::DisplayBuffer& display_buffer)
{
    geometry::Rectangle rect = display_buffer.view_area();

    // XXX Doing rotation here feels wrong, and redundant
    auto orientation = display_buffer.orientation();
    if (orientation == mir_orientation_left ||
        orientation == mir_orientation_right)
    {
        int w = rect.size.width.as_int();
        int h = rect.size.height.as_int();
        rect.size = {h, w};
    }

    auto renderer = renderer_factory->create_renderer_for(rect);
    auto raw = new DefaultDisplayBufferCompositor{display_buffer, scene,
                                                  std::move(renderer),
                                                  report};
    return std::unique_ptr<DisplayBufferCompositor>(raw);
}
