/*
 * Copyright © 2012-2014 Canonical Ltd.
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

#include "mir/graphics/buffer_properties.h"
#include "mir/shell/surface_creation_parameters.h"
#include "surface_stack.h"
#include "mir/compositor/buffer_stream.h"
#include "mir/scene/input_registrar.h"
#include "legacy_surface_change_notification.h"
#include "mir/input/input_channel_factory.h"
#include "mir/scene/scene_report.h"

// TODO Including this doesn't seem right - why would SurfaceStack "know" about BasicSurface
// It is needed by the following member functions:
//  for_each(), for_each_if(), create_surface() and destroy_surface()
// to access:
//  buffer_stream() and input_channel()
#include "basic_surface.h"

#include <boost/throw_exception.hpp>

#include <algorithm>
#include <cassert>
#include <functional>
#include <memory>
#include <stdexcept>

namespace ms = mir::scene;
namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace mi = mir::input;
namespace geom = mir::geometry;

ms::SurfaceStack::SurfaceStack(
    std::shared_ptr<InputRegistrar> const& input_registrar,
    std::shared_ptr<SceneReport> const& report) :
    input_registrar{input_registrar},
    report{report},
    change_cb{[this]() { emit_change_notification(); }},
    notify_change{[]{}}
{
}

mg::RenderableList ms::SurfaceStack::generate_renderable_list() const
{
    std::lock_guard<std::recursive_mutex> lg(guard);
    mg::RenderableList list;
    for (auto &layer : layers_by_depth)
        std::copy(layer.second.begin(), layer.second.end(), std::back_inserter(list));
    return list;
}

void ms::SurfaceStack::for_each_if(mc::FilterForScene& filter, mc::OperatorForScene& op)
{
    std::lock_guard<std::recursive_mutex> lg(guard);
    for (auto &layer : layers_by_depth)
    {
        auto surfaces = layer.second;
        for (auto it = surfaces.begin(); it != surfaces.end(); ++it)
        {
            mg::Renderable& r = **it;
            if (filter(r)) op(r);
        }
    }
}

void ms::SurfaceStack::set_change_callback(std::function<void()> const& f)
{
    std::lock_guard<std::mutex> lg{notify_change_mutex};
    assert(f);
    notify_change = f;
}

void ms::SurfaceStack::add_surface(
    std::shared_ptr<Surface> const& surface,
    DepthId depth,
    mi::InputReceptionMode input_mode)
{
    {
        std::lock_guard<std::recursive_mutex> lg(guard);
        layers_by_depth[depth].push_back(surface);
    }
    input_registrar->input_channel_opened(surface->input_channel(), surface, input_mode);
    report->surface_added(surface.get(), surface.get()->name());
    auto const observer = std::make_shared<LegacySurfaceChangeNotification>(change_cb);
    surface->add_observer(observer);
    emit_change_notification();
}


void ms::SurfaceStack::remove_surface(std::weak_ptr<Surface> const& surface)
{
    auto const keep_alive = surface.lock();

    bool found_surface = false;
    {
        std::lock_guard<std::recursive_mutex> lg(guard);

        for (auto &layer : layers_by_depth)
        {
            auto &surfaces = layer.second;
            auto const p = std::find(surfaces.begin(), surfaces.end(), keep_alive);

            if (p != surfaces.end())
            {
                surfaces.erase(p);
                found_surface = true;
                break;
            }
        }
    }

    if (found_surface)
    {
        input_registrar->input_channel_closed(keep_alive->input_channel());
        report->surface_removed(keep_alive.get(), keep_alive.get()->name());
        emit_change_notification();
    }
    // TODO: error logging when surface not found
}

void ms::SurfaceStack::emit_change_notification()
{
    std::lock_guard<std::mutex> lg{notify_change_mutex};
    notify_change();
}

void ms::SurfaceStack::for_each(std::function<void(std::shared_ptr<mi::InputChannel> const&)> const& callback)
{
    std::lock_guard<std::recursive_mutex> lg(guard);
    for (auto &layer : layers_by_depth)
    {
        for (auto it = layer.second.begin(); it != layer.second.end(); ++it)
            callback((*it)->input_channel());
    }
}

void ms::SurfaceStack::raise(std::weak_ptr<Surface> const& s)
{
    auto surface = s.lock();

    {
        std::unique_lock<std::recursive_mutex> ul(guard);
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

void ms::SurfaceStack::lock()
{
    guard.lock();
}

void ms::SurfaceStack::unlock()
{
    guard.unlock();
}
