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

#include "mir/compositor/buffer_properties.h"
#include "mir/compositor/renderer.h"
#include "mir/shell/surface_creation_parameters.h"
#include "mir/surfaces/surface.h"
#include "mir/surfaces/surface_state.h"
#include "mir/surfaces/surface_stack.h"
#include "mir/surfaces/surface_factory.h"
#include "mir/surfaces/buffer_stream.h"
#include "mir/surfaces/input_registrar.h"
#include "mir/input/input_channel_factory.h"

#include <boost/throw_exception.hpp>

#include <algorithm>
#include <cassert>
#include <functional>
#include <memory>
#include <stdexcept>

namespace ms = mir::surfaces;
namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace mi = mir::input;
namespace geom = mir::geometry;

ms::SurfaceStack::SurfaceStack(std::shared_ptr<SurfaceFactory> const& surface_factory,
                               std::shared_ptr<ms::InputRegistrar> const& input_registrar)
    : surface_factory{surface_factory},
      input_registrar{input_registrar},
      notify_change{[]{}}
{
}

void ms::SurfaceStack::for_each_if(mc::FilterForScene& filter, mc::OperatorForScene& op)
{
    std::lock_guard<std::mutex> lock(guard);
    for (auto &layer : layers_by_depth)
    {
        auto surfaces = layer.second;
        for (auto it = surfaces.begin(); it != surfaces.end(); ++it)
        {
            mc::CompositingCriteria& info = *((*it)->compositing_criteria());
            ms::BufferStream& stream = *((*it)->buffer_stream());
            if (filter(info)) op(info, stream);
        }
    }
}

void ms::SurfaceStack::set_change_callback(std::function<void()> const& f)
{
    std::lock_guard<std::mutex> lock{notify_change_mutex};
    assert(f);
    notify_change = f;
}

std::weak_ptr<ms::Surface> ms::SurfaceStack::create_surface(shell::SurfaceCreationParameters const& params)
{
    auto change_cb = [this]() { emit_change_notification(); };
    auto surface = surface_factory->create_surface(params, change_cb); 
    {
        std::lock_guard<std::mutex> lg(guard);
        layers_by_depth[params.depth].push_back(surface);
    }

    input_registrar->input_channel_opened(surface->input_channel(), surface->input_surface(), params.input_mode);

    emit_change_notification();

    return surface;
}

void ms::SurfaceStack::destroy_surface(std::weak_ptr<ms::Surface> const& surface)
{
    auto keep_alive = surface.lock();
    bool found_surface = false;
    {
        std::lock_guard<std::mutex> lg(guard);

        for (auto &layer : layers_by_depth)
        {
            auto &surfaces = layer.second;
            auto const p = std::find(surfaces.begin(), surfaces.end(), surface.lock());

            if (p != surfaces.end())
            {
                surfaces.erase(p);
                found_surface = true;
                break;
            }
        }
    }
    if (found_surface)
        input_registrar->input_channel_closed(keep_alive->input_channel());
    emit_change_notification();
    // TODO: error logging when surface not found
}

void ms::SurfaceStack::emit_change_notification()
{
    std::lock_guard<std::mutex> lock{notify_change_mutex};
    notify_change();
}

void ms::SurfaceStack::for_each(std::function<void(std::shared_ptr<mi::InputChannel> const&)> const& callback)
{
    std::lock_guard<std::mutex> lg(guard);
    for (auto &layer : layers_by_depth)
    {
        for (auto it = layer.second.begin(); it != layer.second.end(); ++it)
            callback((*it)->input_channel());
    }
}

void ms::SurfaceStack::raise(std::weak_ptr<ms::Surface> const& s)
{
    auto surface = s.lock();

    {
        std::unique_lock<std::mutex> ul(guard);
        for (auto &layer : layers_by_depth)
        {
            auto &surfaces = layer.second;
            auto const p = std::find(surfaces.begin(), surfaces.end(), surface);
    
            if (p != surfaces.end())
            {
                surfaces.erase(p);
                surfaces.push_back(surface);
            
                ul.unlock();
                emit_change_notification();

                return;
            }
        }
    }

    BOOST_THROW_EXCEPTION(std::runtime_error("Invalid surface"));
}
