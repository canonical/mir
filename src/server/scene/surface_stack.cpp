/*
 * Copyright © Canonical Ltd.
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
 */

#include "surface_stack.h"
#include "rendering_tracker.h"
#include "mir/scene/surface.h"
#include "mir/scene/null_surface_observer.h"
#include "mir/scene/scene_report.h"
#include "mir/compositor/scene_element.h"
#include "mir/graphics/renderable.h"
#include "mir/depth_layer.h"
#include "mir/executor.h"
#include "mir/log.h"

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
namespace mf = mir::frontend;
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

ms::SurfaceStack::SurfaceStack(std::shared_ptr<SceneReport> const& report) :
    report{report},
    scene_changed{false},
    surface_observer{std::make_shared<SurfaceDepthLayerObserver>(this)},
    multiplexer(linearising_executor)
{
}

ms::SurfaceStack::~SurfaceStack() noexcept(true)
{
    RecursiveWriteLock lg(guard);
    for (auto const& layer : surface_layers)
    {
        for (auto const& surface : layer)
        {
            surface->unregister_interest(*surface_observer);
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
            if (surface_can_be_shown(surface) && surface->visible())
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
        surface->register_interest(surface_observer, immediate_executor);
    }
    surface->set_reception_mode(input_mode);
    observers.surface_added(surface);

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
                keep_alive->unregister_interest(*surface_observer);
                found_surface = true;
                break;
            }
        }
    }

    if (found_surface)
    {
        observers.surface_removed(keep_alive);
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
            if (surface_can_be_shown(surface) && surface->input_area_contains(cursor))
                    return surface;
        }
    }

    return {};
}

auto ms::SurfaceStack::input_surface_at(geometry::Point point) const -> std::shared_ptr<input::Surface>
{
    return surface_at(point);
}

void ms::SurfaceStack::raise(Surface const* surface)
{
    SurfaceSet affected_surfaces;

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
                affected_surfaces.insert(surface_shared);
                break;
            }
        }
    }

    if (affected_surfaces.empty())
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Invalid surface"));
    }
    else
    {
        observers.surfaces_reordered(affected_surfaces);
    }

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
    {
        observers.surfaces_reordered(ss);
    }
}

void ms::SurfaceStack::swap_z_order(SurfaceSet const& first, SurfaceSet const& second)
{
    {
        RecursiveWriteLock ul(guard);
        for (auto& layer : surface_layers)
        {
            // The goal is to swap the first set with the second set such that their Z-order is swapped.

            // First, find the start and end of the range that we want to swap
            auto swap_begin = layer.begin();
            auto swap_end = layer.end();
            size_t num_first_found = 0;
            size_t num_second_found = 0;
            bool first_to_front = false;
            for (auto it = layer.begin(); it != layer.end(); it++)
            {
                // Once we've found them all, then we have the whole range to sort across
                if (num_first_found == first.size() && num_second_found == second.size())
                {
                    swap_end = it;
                    break;
                }

                // Find the start position and count how many we've found
                bool in_first = first.count(*it) > 0;
                bool in_second = second.contains(*it) > 0;
                if (in_first)
                {
                    if (!num_first_found && !num_second_found)
                        swap_begin = it;

                    num_first_found++;
                }
                else if (in_second)
                {
                    if (!num_first_found && !num_second_found)
                    {
                        first_to_front = true;
                        swap_begin = it;
                    }

                    num_second_found++;
                }
            }

            // Finally, move the to_front items to the front of the group and the to_back to the back of the group
            auto const& to_front = first_to_front ? first : second;
            auto const& to_back = first_to_front ? second : first;
            std::stable_sort(swap_begin, swap_end, [&](std::weak_ptr<Surface> const& s1, std::weak_ptr<Surface> const& s2)
            {
                if (to_front.count(s1))
                    return to_front.count(s2) == 0;
                else if (to_back.count(s1) || to_front.count(s2))
                    return false;
                else
                    return to_back.count(s2) == 0;
            });
        }
    }

    observers.surfaces_reordered(first);
    observers.surfaces_reordered(second);
}

void ms::SurfaceStack::send_to_back(const mir::scene::SurfaceSet &ss)
{
    bool surfaces_reordered{false};
    {
        RecursiveWriteLock ul(guard);
        for (auto& layer : surface_layers)
        {
            // Only reorder if "layer" contains at least one surface
            // in "ss" and the surface(s) are not already at the beginning
            auto it = layer.begin();
            for (; it != layer.end(); it++)
                if (!ss.count(*it))
                    break;

            bool needs_reorder = false;
            for (; it != layer.end(); it++)
                if (ss.count(*it))
                    needs_reorder = true;

            if (needs_reorder)
            {
                // "Back" in Z-order will be the front of the list.
                std::stable_partition(
                    begin(layer), end(layer),
                    [&](std::weak_ptr<Surface> const& s) { return ss.count(s); });
                surfaces_reordered = true;
            }
        }
    }

    if (surfaces_reordered)
    {
        observers.surfaces_reordered(ss);
    }
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
    unsigned int depth_index = mir_depth_layer_get_index(surface->depth_layer());
    if (surface_layers.size() <= depth_index)
        surface_layers.resize(depth_index + 1);
    surface_layers[depth_index].push_back(surface);
}

auto ms::SurfaceStack::surface_can_be_shown(std::shared_ptr<Surface> const& surface) const -> bool
{
    return !is_locked || surface->visible_on_lock_screen();
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
            observer->surface_exists(surface);
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

auto ms::SurfaceStack::stacking_order_of(SurfaceSet const& surfaces) const -> SurfaceList
{
    SurfaceList result;

    RecursiveReadLock lk(guard);
    for (auto const& layer : surface_layers)
    {
        for (auto const& surface : layer)
        {
            if (surfaces.find(surface) != surfaces.end())
            {
                result.push_back(surface);
            }
        }
    }
    return result;
}

auto ms::SurfaceStack::screen_is_locked() const -> bool
{
    return is_locked;
}

void ms::SurfaceStack::lock()
{
    bool temp = false;
    if (is_locked.compare_exchange_strong(temp, true))
    {
        mir::log_info("Session is now locked and listeners are being notified");
        emit_scene_changed();
        multiplexer.on_lock();
    }
    else
    {
        mir::log_debug("Session received duplicate lock request");
    }
}

void ms::SurfaceStack::unlock()
{
    bool temp = true;
    if (is_locked.compare_exchange_strong(temp, false))
    {
        mir::log_info("Session is now unlocked and listeners are being notified");
        emit_scene_changed();
        multiplexer.on_unlock();
    }
    else
    {
        mir::log_debug("Session received duplicate unlock request");
    }
}

void ms::SurfaceStack::register_interest(std::weak_ptr<SessionLockObserver> const& observer)
{
    multiplexer.register_interest(observer);
}

void ms::SurfaceStack::register_interest(
    std::weak_ptr<SessionLockObserver> const& observer,
    mir::Executor& other_executor)
{
    multiplexer.register_interest(observer, other_executor);
}

void ms::SurfaceStack::register_early_observer(
    std::weak_ptr<SessionLockObserver> const& observer,
    mir::Executor& other_executor)
{
    multiplexer.register_early_observer(observer, other_executor);
}

void ms::SurfaceStack::unregister_interest(SessionLockObserver const& observer)
{
    multiplexer.unregister_interest(observer);
}

void ms::Observers::surface_added(std::shared_ptr<Surface> const& surface)
{
    for_each([&](std::shared_ptr<Observer> const& observer)
        { observer->surface_added(surface); });
}

void ms::Observers::surface_removed(std::shared_ptr<Surface> const& surface)
{
    for_each([&](std::shared_ptr<Observer> const& observer)
        { observer->surface_removed(surface); });
}

void ms::Observers::surfaces_reordered(SurfaceSet const& affected_surfaces)
{
    for_each([&](std::shared_ptr<Observer> const& observer)
        { observer->surfaces_reordered(affected_surfaces); });
}

void ms::Observers::scene_changed()
{
   for_each([&](std::shared_ptr<Observer> const& observer)
        { observer->scene_changed(); });
}

void ms::Observers::surface_exists(std::shared_ptr<Surface> const& surface)
{
    for_each([&](std::shared_ptr<Observer> const& observer)
        { observer->surface_exists(surface); });
}

void ms::Observers::end_observation()
{
    for_each([&](std::shared_ptr<Observer> const& observer)
        { observer->end_observation(); });
}

ms::SessionLockObserverMultiplexer::SessionLockObserverMultiplexer(Executor& executor)
    : ObserverMultiplexer<SessionLockObserver>(executor)
{
}

void ms::SessionLockObserverMultiplexer::on_lock()
{
    for_each_observer(&SessionLockObserver::on_lock);
}

void ms::SessionLockObserverMultiplexer::on_unlock()
{
    for_each_observer(&SessionLockObserver::on_unlock);
}
