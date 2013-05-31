/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *   Alan Griffiths <alan@octopull.co.uk>
 *   Thomas Voss <thomas.voss@canonical.com>
 */

#include "mir/surfaces/buffer_bundle_factory.h"
#include "mir/compositor/buffer_properties.h"
#include "mir/graphics/renderer.h"
#include "mir/shell/surface_creation_parameters.h"
#include "mir/surfaces/surface.h"
#include "mir/surfaces/surface_stack.h"
#include "mir/surfaces/input_registrar.h"
#include "mir/input/input_channel_factory.h"

#include <algorithm>
#include <cassert>
#include <functional>
#include <memory>

namespace ms = mir::surfaces;
namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace mi = mir::input;
namespace geom = mir::geometry;

ms::SurfaceStack::SurfaceStack(std::shared_ptr<BufferBundleFactory> const& bb_factory,
                               std::shared_ptr<mi::InputChannelFactory> const& input_factory,
                               std::shared_ptr<ms::InputRegistrar> const& input_registrar)
    : buffer_bundle_factory{bb_factory},
      input_factory{input_factory},
      input_registrar{input_registrar},
      notify_change{[]{}}
{
    assert(buffer_bundle_factory);
}

void ms::SurfaceStack::for_each_if(mc::FilterForRenderables& filter, mc::OperatorForRenderables& renderable_operator)
{
    std::lock_guard<std::mutex> lock(guard);
    for (auto it = surfaces.rbegin(); it != surfaces.rend(); ++it)
    {
        mg::Renderable& renderable = **it;
        if (filter(renderable)) renderable_operator(renderable);
    }
}

void ms::SurfaceStack::set_change_callback(std::function<void()> const& f)
{
    std::lock_guard<std::mutex> lock{notify_change_mutex};
    assert(f);
    notify_change = f;
}

std::weak_ptr<ms::Surface> ms::SurfaceStack::create_surface(const shell::SurfaceCreationParameters& params)
{

    mc::BufferProperties buffer_properties{params.size,
                                           params.pixel_format,
                                           params.buffer_usage};

    std::shared_ptr<ms::Surface> surface(
        new ms::Surface(
            params.name, params.top_left,
            buffer_bundle_factory->create_buffer_bundle(buffer_properties),
            input_factory->make_input_channel(),
            [this]() { emit_change_notification(); }));
    
    {
        std::lock_guard<std::mutex> lg(guard);
        surfaces.push_back(surface);
    }

    // TODO: It might be a nice refactoring to combine this with input channel creation
    // i.e. client_fd = registrar->register_for_input(surface). ~racarr
    input_registrar->input_surface_opened(surface);

    emit_change_notification();

    return surface;
}

void ms::SurfaceStack::destroy_surface(std::weak_ptr<ms::Surface> const& surface)
{
    {
        std::lock_guard<std::mutex> lg(guard);

        auto const p = std::find(surfaces.begin(), surfaces.end(), surface.lock());
        
        if (p != surfaces.end())
        {
            input_registrar->input_surface_closed(*p);
            surfaces.erase(p);
        }
        // else; TODO error logging
    }

    emit_change_notification();
}

void ms::SurfaceStack::emit_change_notification()
{
    std::lock_guard<std::mutex> lock{notify_change_mutex};
    notify_change();
}

void ms::SurfaceStack::for_each(std::function<void(std::shared_ptr<mi::SurfaceTarget> const&)> const& callback)
{
    for (auto it = surfaces.rbegin(); it != surfaces.rend(); ++it)
        callback(*it);
}
