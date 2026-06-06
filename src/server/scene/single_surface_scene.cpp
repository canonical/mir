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

#include <mir/scene/surface_scene.h>
#include <mir/compositor/scene.h>
#include <mir/compositor/scene_element.h>
#include <mir/scene/observer.h>
#include <mir/scene/surface.h>
#include "rendering_tracker.h"
#include "surface_stack.h"
#include "surface_scene_element.h"

#include <boost/throw_exception.hpp>

namespace ms = mir::scene;
namespace mc = mir::compositor;

namespace mir::scene
{
class SingleSurfaceScene :
    public compositor::Scene
{
public:
    SingleSurfaceScene(std::weak_ptr<Surface> const& surface);

    // From Scene
    auto scene_elements_for(mc::CompositorID id) -> mc::SceneElementSequence override;
    void register_compositor(mc::CompositorID id) override;
    void unregister_compositor(mc::CompositorID id) override;

    void add_observer(std::shared_ptr<Observer> const& observer) override;
    void remove_observer(std::weak_ptr<Observer> const& observer) override;

private:
    SingleSurfaceScene(const SingleSurfaceScene&) = delete;
    SingleSurfaceScene& operator=(const SingleSurfaceScene&) = delete;

    RecursiveReadWriteMutex guard;
    std::weak_ptr<Surface> surface;
    std::shared_ptr<RenderingTracker> rendering_tracker;
    std::set<compositor::CompositorID> registered_compositors;

    Observers observers;
};
}

ms::SingleSurfaceScene::SingleSurfaceScene(
    std::weak_ptr<Surface> const &surface)
    : surface{surface},
      rendering_tracker{std::make_shared<ms::RenderingTracker>(surface)}
{
}

auto ms::SingleSurfaceScene::scene_elements_for(mc::CompositorID id) -> mc::SceneElementSequence
{
    RecursiveReadLock lg(guard);

    mc::SceneElementSequence elements;
    if (auto const locked = surface.lock())
    {
        for (auto& renderable : locked->generate_renderables(id))
        {
            elements.emplace_back(
                std::make_shared<SurfaceSceneElement>(
                    locked->name(),
                    renderable,
                    rendering_tracker,
                    id));
        }
    }
    return elements;
}

void ms::SingleSurfaceScene::register_compositor(mc::CompositorID id)
{
    RecursiveWriteLock lg(guard);

    registered_compositors.insert(id);
    rendering_tracker->active_compositors(registered_compositors);
}

void ms::SingleSurfaceScene::unregister_compositor(mc::CompositorID id)
{
    RecursiveWriteLock lg(guard);

    registered_compositors.erase(id);
    rendering_tracker->active_compositors(registered_compositors);
}

void ms::SingleSurfaceScene::add_observer(std::shared_ptr<ms::Observer> const& observer)
{
    observers.add(observer);

    // Notify observer of our surface.
    RecursiveReadLock lg(guard);
    if (auto const locked = surface.lock())
        observer->surface_exists(locked);
}

void ms::SingleSurfaceScene::remove_observer(std::weak_ptr<ms::Observer> const& observer)
{
    auto o = observer.lock();
    if (!o)
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid observer (destroyed)"));

    o->end_observation();

    observers.remove(o);
}

auto ms::create_surface_scene(std::weak_ptr<Surface> const& surface) -> std::shared_ptr<mc::Scene>
{
    return std::make_shared<SingleSurfaceScene>(surface);
}
