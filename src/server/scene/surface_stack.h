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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_SCENE_SURFACE_STACK_H_
#define MIR_SCENE_SURFACE_STACK_H_

#include "surface_stack_model.h"

#include "mir/compositor/scene.h"
#include "mir/scene/depth_id.h"
#include "mir/scene/observer.h"
#include "mir/input/scene.h"

#include "mir/basic_observers.h"

#include <memory>
#include <vector>
#include <mutex>
#include <map>
#include <set>

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
class InputRegistrar;
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

class SurfaceStack : public compositor::Scene, public input::Scene, public SurfaceStackModel
{
public:
    explicit SurfaceStack(
        std::shared_ptr<SceneReport> const& report);
    virtual ~SurfaceStack() noexcept(true) {}

    // From Scene
    compositor::SceneElementSequence scene_elements_for(compositor::CompositorID id) override;
    int frames_pending(compositor::CompositorID) const override;
    void register_compositor(compositor::CompositorID id) override;
    void unregister_compositor(compositor::CompositorID id) override;

    // From Scene
    void for_each(std::function<void(std::shared_ptr<input::Surface> const&)> const& callback) override;

    virtual void remove_surface(std::weak_ptr<Surface> const& surface) override;

    virtual void raise(std::weak_ptr<Surface> const& surface) override;

    void raise(SurfaceSet const& surfaces) override;

    void add_surface(
        std::shared_ptr<Surface> const& surface,
        DepthId depth,
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

    std::mutex mutable guard;

    std::shared_ptr<InputRegistrar> const input_registrar;
    std::shared_ptr<SceneReport> const report;

    typedef std::vector<std::shared_ptr<Surface>> Layer;
    std::map<DepthId, Layer> layers_by_depth;
    std::map<Surface*,std::shared_ptr<RenderingTracker>> rendering_trackers;
    std::set<compositor::CompositorID> registered_compositors;
    
    std::vector<std::shared_ptr<graphics::Renderable>> overlays;

    Observers observers;
    bool scene_changed = false;
};

}
}

#endif /* MIR_SCENE_SURFACE_STACK_H_ */
