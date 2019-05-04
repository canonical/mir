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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_SCENE_SURFACE_STACK_H_
#define MIR_SCENE_SURFACE_STACK_H_

#include "mir/shell/surface_stack.h"

#include "mir/compositor/scene.h"
#include "mir/scene/observer.h"
#include "mir/input/scene.h"
#include "mir/recursive_read_write_mutex.h"

#include "mir/basic_observers.h"
#include "mir/scene/surface_observer.h"

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <vector>

namespace mir
{
namespace graphics
{
class Renderable;
}
/// Management of Surface objects. Includes the model (SurfaceStack and Surface
/// classes) and controller (SurfaceController) elements of an MVC design.
namespace scene
{
class BasicSurface;
class SceneReport;
class RenderingTracker;

class Observers : public Observer, BasicObservers<Observer>
{
public:
   // ms::Observer
   void surface_added(Surface* surface) override;
   void surface_removed(Surface* surface) override;
   void surfaces_reordered() override;
   void scene_changed() override;
   void surface_exists(Surface* surface) override;
   void end_observation() override;

   using BasicObservers<Observer>::add;
   using BasicObservers<Observer>::remove;
};

class SurfaceStack : public compositor::Scene, public input::Scene, public shell::SurfaceStack
{
public:
    explicit SurfaceStack(
        std::shared_ptr<SceneReport> const& report);
    virtual ~SurfaceStack() noexcept(true);

    // From Scene
    compositor::SceneElementSequence scene_elements_for(compositor::CompositorID id) override;
    int frames_pending(compositor::CompositorID) const override;
    void register_compositor(compositor::CompositorID id) override;
    void unregister_compositor(compositor::CompositorID id) override;

    // From Scene
    void for_each(std::function<void(std::shared_ptr<input::Surface> const&)> const& callback) override;

    virtual void remove_surface(std::weak_ptr<Surface> const& surface) override;

    void raise(Surface const* surface);
    virtual void raise(std::weak_ptr<Surface> const& surface) override;
    void raise(SurfaceSet const& surfaces) override;

    void add_surface(
        std::shared_ptr<Surface> const& surface,
        input::InputReceptionMode input_mode) override;

    auto surface_at(geometry::Point) const -> std::shared_ptr<Surface> override;

    void add_observer(std::shared_ptr<Observer> const& observer) override;
    void remove_observer(std::weak_ptr<Observer> const& observer) override;

    // Intended for input overlays, as described in mir::input::Scene documentation.
    void add_input_visualization(std::shared_ptr<graphics::Renderable> const& overlay) override;
    void remove_input_visualization(std::weak_ptr<graphics::Renderable> const& overlay) override;

    void emit_scene_changed() override;

private:
    SurfaceStack(const SurfaceStack&) = delete;
    SurfaceStack& operator=(const SurfaceStack&) = delete;
    void create_rendering_tracker_for(std::shared_ptr<Surface> const&);
    void update_rendering_tracker_compositors();
    void insert_surface_at_top_of_depth_layer(std::shared_ptr<Surface> const& surface);

    /**
     * Converts a MirDepthLayer into an index for a list of layers (0 is bottom-most layer)
     */
    static auto mir_depth_layer_to_index(MirDepthLayer depth_layer) -> unsigned int;

    RecursiveReadWriteMutex mutable guard;

    std::shared_ptr<SceneReport> const report;

    /**
     * All surfaces managed by this class
     *
     * Each depth layer is mapped to an index of the outer vector by mir_depth_layer_to_index()
     * The outer vector starts out empty, and is expanded as needed to contain the highest layer encountered
     * The inner vectors contain the list of surfaces on each layer (bottom to top)
     */
    std::vector<std::vector<std::shared_ptr<Surface>>> surface_layers;
    std::map<Surface*,std::shared_ptr<RenderingTracker>> rendering_trackers;
    std::set<compositor::CompositorID> registered_compositors;
    
    std::vector<std::shared_ptr<graphics::Renderable>> overlays;

    Observers observers;
    std::atomic<bool> scene_changed;
    std::shared_ptr<SurfaceObserver> surface_observer;
};

}
}

#endif /* MIR_SCENE_SURFACE_STACK_H_ */
