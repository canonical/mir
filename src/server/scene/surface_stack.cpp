/*
 * Copyright © 2012-2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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

#include "surface_stack.h"
#include "rendering_tracker.h"
#include "mir/scene/surface.h"
#include "mir/scene/null_surface_observer.h"
#include "mir/scene/scene_report.h"
#include "mir/compositor/scene_element.h"
#include "mir/graphics/renderable.h"

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

namespace
{

class SurfaceSceneElement : public mc::SceneElement
{
public:
    SurfaceSceneElement(
        std::string name,
        std::shared_ptr<mg::Renderable> const& renderable,
        std::shared_ptr<ms::RenderingTracker> const& tracker,
        mc::CompositorID id)
        : renderable_{renderable},
          tracker{tracker},
          cid{id},
          surface_name(name)
    {
    }

    std::shared_ptr<mg::Renderable> renderable() const override
    {
        return renderable_;
    }

    void rendered() override
    {
        tracker->rendered_in(cid);
    }

    void occluded() override
    {
        tracker->occluded_in(cid);
    }

private:
    std::shared_ptr<mg::Renderable> const renderable_;
    std::shared_ptr<ms::RenderingTracker> const tracker;
    mc::CompositorID cid;
    std::string const surface_name;
};

//note: something different than a 2D/HWC overlay
class OverlaySceneElement : public mc::SceneElement
{
public:
    OverlaySceneElement(
        std::shared_ptr<mg::Renderable> renderable)
        : renderable_{renderable}
    {
    }

    std::shared_ptr<mg::Renderable> renderable() const override
    {
        return renderable_;
    }

    void rendered() override
    {
    }

    void occluded() override
    {
    }

private:
    std::shared_ptr<mg::Renderable> const renderable_;
};

/**
 * A SurfaceDepthLayerObserver must not outlive the SurfaceStack it was created for
 */
struct SurfaceDepthLayerObserver : ms::NullSurfaceObserver
{
    SurfaceDepthLayerObserver(ms::SurfaceStack* stack)
        : stack{stack}
    {
    }

    void depth_layer_set_to(ms::Surface const* surface, MirDepthLayer /*z_index*/) override
    {
        // move the surface to the top of it's new layer
        stack->raise(surface);
    }

private:
    ms::SurfaceStack* stack;
};

}

ms::SurfaceStack::SurfaceStack(
    std::shared_ptr<SceneReport> const& report) :
    report{report},
    scene_changed{false},
    surface_observer{std::make_shared<SurfaceDepthLayerObserver>(this)}
{
}

ms::SurfaceStack::~SurfaceStack() noexcept(true)
{
    RecursiveWriteLock lg(guard);
    for (auto const& layer : surface_layers)
    {
        for (auto const& surface : layer)
        {
            surface->remove_observer(surface_observer);
        }
    }
}

mc::SceneElementSequence ms::SurfaceStack::scene_elements_for(mc::CompositorID id)
{
    RecursiveReadLock lg(guard);

    scene_changed = false;
    mc::SceneElementSequence elements;
    for (auto const& layer : surface_layers)
    {
        for (auto const& surface : layer)
        {
            if (surface->visible())
            {
                for (auto& renderable : surface->generate_renderables(id))
                {
                    elements.emplace_back(
                        std::make_shared<SurfaceSceneElement>(
                            surface->name(),
                            renderable,
                            rendering_trackers[surface.get()],
                            id));
                }
            }
        }
    }
    for (auto const& renderable : overlays)
    {
        elements.emplace_back(std::make_shared<OverlaySceneElement>(renderable));
    }
    return elements;
}

int ms::SurfaceStack::frames_pending(mc::CompositorID id) const
{
    RecursiveReadLock lg(guard);

    int result = scene_changed ? 1 : 0;
    for (auto const& layer : surface_layers)
    {
        for (auto const& surface : layer)
        {
            if (surface->visible())
            {
                auto const tracker = rendering_trackers.find(surface.get());
                if (tracker != rendering_trackers.end() && tracker->second->is_exposed_in(id))
                {
                    // Note that we ask the surface and not a Renderable.
                    // This is because we don't want to waste time and resources
                    // on a snapshot till we're sure we need it...
                    int ready = surface->buffers_ready_for_compositor(id);
                    if (ready > result)
                        result = ready;
                }
            }
        }
    }
    return result;
}

void ms::SurfaceStack::register_compositor(mc::CompositorID cid)
{
    RecursiveWriteLock lg(guard);

    registered_compositors.insert(cid);

    update_rendering_tracker_compositors();
}

void ms::SurfaceStack::unregister_compositor(mc::CompositorID cid)
{
    RecursiveWriteLock lg(guard);

    registered_compositors.erase(cid);

    update_rendering_tracker_compositors();
}

void ms::SurfaceStack::add_input_visualization(
    std::shared_ptr<mg::Renderable> const& overlay)
{
    {
        RecursiveWriteLock lg(guard);
        overlays.push_back(overlay);
    }
    emit_scene_changed();
}

void ms::SurfaceStack::remove_input_visualization(
    std::weak_ptr<mg::Renderable> const& weak_overlay)
{
    auto overlay = weak_overlay.lock();
    {
        RecursiveWriteLock lg(guard);
        auto const p = std::find(overlays.begin(), overlays.end(), overlay);
        if (p == overlays.end())
        {
            BOOST_THROW_EXCEPTION(std::runtime_error("Attempt to remove an overlay which was never added or which has been previously removed"));
        }
        overlays.erase(p);
    }
    
    emit_scene_changed();
}

void ms::SurfaceStack::emit_scene_changed()
{
    {
        RecursiveWriteLock lg(guard);
        scene_changed = true;
    }
    observers.scene_changed();
}

void ms::SurfaceStack::add_surface(
    std::shared_ptr<Surface> const& surface,
    mi::InputReceptionMode input_mode)
{
    {
        RecursiveWriteLock lg(guard);
        insert_surface_at_top_of_depth_layer(surface);
        create_rendering_tracker_for(surface);
        surface->add_observer(surface_observer);
    }
    surface->set_reception_mode(input_mode);
    observers.surface_added(surface.get());

    report->surface_added(surface.get(), surface.get()->name());
}

void ms::SurfaceStack::remove_surface(std::weak_ptr<Surface> const& surface)
{
    auto const keep_alive = surface.lock();

    bool found_surface = false;
    {
        RecursiveWriteLock lg(guard);

        for (auto& layer : surface_layers)
        {
            auto const surface = std::find(layer.begin(), layer.end(), keep_alive);

            if (surface != layer.end())
            {
                layer.erase(surface);
                rendering_trackers.erase(keep_alive.get());
                keep_alive->remove_observer(surface_observer);
                found_surface = true;
                break;
            }
        }
    }

    if (found_surface)
    {
        observers.surface_removed(keep_alive.get());
        report->surface_removed(keep_alive.get(), keep_alive.get()->name());
    }
    // TODO: error logging when surface not found
}

namespace
{
template <typename Container>
struct InReverse {
    Container& container;
    auto begin() -> decltype(container.rbegin()) { return container.rbegin(); }
    auto end() -> decltype(container.rend()) { return container.rend(); }
};

template <typename Container>
InReverse<Container> in_reverse(Container& container) { return InReverse<Container>{container}; }
}

auto ms::SurfaceStack::surface_at(geometry::Point cursor) const
-> std::shared_ptr<Surface>
{
    RecursiveReadLock lg(guard);
    for (auto const& layer : in_reverse(surface_layers))
    {
        for (auto const& surface : in_reverse(layer))
        {
            // TODO There's a lack of clarity about how the input area will
            // TODO be maintained and whether this test will detect clicks on
            // TODO decorations (it should) as these may be outside the area
            // TODO known to the client.  But it works for now.
            if (surface->input_area_contains(cursor))
                    return surface;
        }
    }

    return {};
}

void ms::SurfaceStack::for_each(std::function<void(std::shared_ptr<mi::Surface> const&)> const& callback)
{
    RecursiveReadLock lg(guard);
    for (auto const& layer : surface_layers)
    {
        for (auto const& surface : layer)
        {
            callback(surface);
        }
    }
}

void ms::SurfaceStack::raise(Surface const* surface)
{
    bool surfaces_reordered{false};

    {
        RecursiveWriteLock ul(guard);
        for (auto& layer : surface_layers)
        {
            auto const p = std::find_if(
                layer.begin(),
                layer.end(),
                [surface](auto const& i)
                    {
                        return surface == i.get();
                    });

            if (p != layer.end())
            {
                std::shared_ptr<Surface> surface_shared = *p;
                layer.erase(p);
                insert_surface_at_top_of_depth_layer(surface_shared);
                surfaces_reordered = true;
                break;
            }
        }
    }

    if (!surfaces_reordered)
        BOOST_THROW_EXCEPTION(std::runtime_error("Invalid surface"));

    observers.surfaces_reordered();
}

void ms::SurfaceStack::raise(std::weak_ptr<Surface> const& s)
{
    raise(s.lock().get());
}

void ms::SurfaceStack::raise(SurfaceSet const& ss)
{
    bool surfaces_reordered{false};
    {
        RecursiveWriteLock ul(guard);
        for (auto& layer : surface_layers)
        {
            auto const old_layer = layer;

            // Put all the surfaces to raise at the end of the list (preserving order)
            auto split = std::stable_partition(
                begin(layer), end(layer),
                [&](std::weak_ptr<Surface> const& s) { return !ss.count(s); });

            // Make a new vector with only the surfaces to raise
            auto to_raise = std::vector<std::shared_ptr<Surface>>{split, layer.end()};

            // Chop off the surfaces we are moving from the old vector (they are now only in to_raise)
            layer.erase(split, layer.end());

            // One by one insert to_raise surfaces into the surfaces vector at the correct position
            // It is important that to_raise is still in the original order
            for (auto const& surface : to_raise)
                insert_surface_at_top_of_depth_layer(surface);

            // Only set surfaces_reordered if the end result is different than before
            if (old_layer != layer)
                surfaces_reordered = true;
        }
    }

    if (surfaces_reordered)
        observers.surfaces_reordered();
}

void ms::SurfaceStack::create_rendering_tracker_for(std::shared_ptr<Surface> const& surface)
{
    auto const tracker = std::make_shared<RenderingTracker>(surface);

    RecursiveWriteLock ul(guard);
    tracker->active_compositors(registered_compositors);
    rendering_trackers[surface.get()] = tracker;
}

void ms::SurfaceStack::update_rendering_tracker_compositors()
{
    RecursiveReadLock ul(guard);

    for (auto const& pair : rendering_trackers)
        pair.second->active_compositors(registered_compositors);
}

void ms::SurfaceStack::insert_surface_at_top_of_depth_layer(std::shared_ptr<Surface> const& surface)
{
    unsigned int depth_index = mir_depth_layer_to_index(surface->depth_layer());
    if (surface_layers.size() <= depth_index)
        surface_layers.resize(depth_index + 1);
    surface_layers[depth_index].push_back(surface);
}

auto ms::SurfaceStack::mir_depth_layer_to_index(MirDepthLayer depth_layer) -> unsigned int
{
    switch (depth_layer)
    {
    case mir_depth_layer_background:        return 0;
    case mir_depth_layer_below:             return 1;
    case mir_depth_layer_application:       return 2;
    case mir_depth_layer_always_on_top:     return 3;
    case mir_depth_layer_above:             return 4;
    case mir_depth_layer_overlay:           return 5;
    }
    assert(false);
}

void ms::SurfaceStack::add_observer(std::shared_ptr<ms::Observer> const& observer)
{
    observers.add(observer);

    // Notify observer of existing surfaces
    RecursiveReadLock lk(guard);
    for (auto const& layer : surface_layers)
    {
        for (auto const& surface : layer)
        {
            observer->surface_exists(surface.get());
        }
    }
}

void ms::SurfaceStack::remove_observer(std::weak_ptr<ms::Observer> const& observer)
{
    auto o = observer.lock();
    if (!o)
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid observer (destroyed)"));
    
    o->end_observation();
    
    observers.remove(o);
}

void ms::Observers::surface_added(ms::Surface* surface) 
{
    for_each([&](std::shared_ptr<Observer> const& observer)
        { observer->surface_added(surface); });
}

void ms::Observers::surface_removed(ms::Surface* surface)
{
    for_each([&](std::shared_ptr<Observer> const& observer)
        { observer->surface_removed(surface); });
}

void ms::Observers::surfaces_reordered()
{
    for_each([&](std::shared_ptr<Observer> const& observer)
        { observer->surfaces_reordered(); });
}

void ms::Observers::scene_changed()
{
   for_each([&](std::shared_ptr<Observer> const& observer)
        { observer->scene_changed(); });
}

void ms::Observers::surface_exists(ms::Surface* surface)
{
    for_each([&](std::shared_ptr<Observer> const& observer)
        { observer->surface_exists(surface); });
}

void ms::Observers::end_observation()
{
    for_each([&](std::shared_ptr<Observer> const& observer)
        { observer->end_observation(); });
}
