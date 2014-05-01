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

#ifndef MIR_SCENE_SCENETACK_H_
#define MIR_SCENE_SCENETACK_H_

#include "surface_stack_model.h"

#include "mir/compositor/scene.h"
#include "mir/scene/depth_id.h"
#include "mir/scene/observer.h"
#include "mir/input/input_targets.h"

#include <memory>
#include <vector>
#include <mutex>
#include <map>

namespace mir
{
namespace compositor
{
class FilterForScene;
class OperatorForScene;
}

namespace frontend
{
struct SurfaceCreationParameters;
}

namespace input
{
class InputChannelFactory;
class Surface;
}

/// Management of Surface objects. Includes the model (SurfaceStack and Surface
/// classes) and controller (SurfaceController) elements of an MVC design.
namespace scene
{
class InputRegistrar;
class BasicSurface;
class SceneReport;

class Observers : public Observer
{
public:
   // ms::Observer
   void surface_added(Surface* surface) override;
   void surface_removed(Surface* surface) override;
   void surfaces_reordered() override;
   void surface_exists(Surface* surface) override;
   void end_observation();

   void add_observer(std::shared_ptr<Observer> const& observer);
   void remove_observer(std::shared_ptr<Observer> const& observer);

private:
    std::mutex mutex;
    std::vector<std::shared_ptr<Observer>> observers;
};

class SurfaceStack : public compositor::Scene, public input::InputTargets, public SurfaceStackModel
{
public:
    explicit SurfaceStack(
        std::shared_ptr<InputRegistrar> const& input_registrar,
        std::shared_ptr<SceneReport> const& report);
    virtual ~SurfaceStack() noexcept(true) {}

    // From Scene
    graphics::RenderableList renderable_list_for(CompositorID id) const;
    
    // From InputTargets
    void for_each(std::function<void(std::shared_ptr<input::Surface> const&)> const& callback);

    virtual void remove_surface(std::weak_ptr<Surface> const& surface) override;

    virtual void raise(std::weak_ptr<Surface> const& surface) override;

    void add_surface(
        std::shared_ptr<Surface> const& surface,
        DepthId depth,
        input::InputReceptionMode input_mode) override;
    
    void add_observer(std::shared_ptr<Observer> const& observer) override;
    void remove_observer(std::weak_ptr<Observer> const& observer) override;

private:
    SurfaceStack(const SurfaceStack&) = delete;
    SurfaceStack& operator=(const SurfaceStack&) = delete;

    std::mutex mutable guard;

    std::shared_ptr<InputRegistrar> const input_registrar;
    std::shared_ptr<SceneReport> const report;

    typedef std::vector<std::shared_ptr<Surface>> Layer;
    std::map<DepthId, Layer> layers_by_depth;

    Observers observers;
};

}
}

#endif /* MIR_SCENE_SCENETACK_H_ */
