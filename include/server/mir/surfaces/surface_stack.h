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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_SURFACES_SURFACESTACK_H_
#define MIR_SURFACES_SURFACESTACK_H_

#include "surface_stack_model.h"

#include "mir/compositor/scene.h"
#include "mir/surfaces/depth_id.h"
#include "mir/input/input_targets.h"

#include <memory>
#include <vector>
#include <mutex>
#include <map>

namespace mir
{
namespace compositor
{
class RenderableCollection;
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
class InputChannel;
}

/// Management of Surface objects. Includes the model (SurfaceStack and Surface
/// classes) and controller (SurfaceController) elements of an MVC design.
namespace surfaces
{
class SurfaceFactory;
class InputRegistrar;
class Surface;

class SurfaceStack : public compositor::Scene, public input::InputTargets, public SurfaceStackModel
{
public:
    explicit SurfaceStack(std::shared_ptr<SurfaceFactory> const& surface_factory,
                          std::shared_ptr<InputRegistrar> const& input_registrar);
    virtual ~SurfaceStack() noexcept(true) {}

    // From Scene
    virtual void for_each_if(compositor::FilterForScene &filter, compositor::OperatorForScene &op);
    virtual void set_change_callback(std::function<void()> const& f);
    
    // From InputTargets
    void for_each(std::function<void(std::shared_ptr<input::InputChannel> const&)> const& callback);

    // From SurfaceStackModel 
    virtual std::weak_ptr<Surface> create_surface(const shell::SurfaceCreationParameters& params);

    virtual void destroy_surface(std::weak_ptr<Surface> const& surface);
    
    virtual void raise(std::weak_ptr<Surface> const& surface);

    virtual void lock();
    virtual void unlock();

private:
    SurfaceStack(const SurfaceStack&) = delete;
    SurfaceStack& operator=(const SurfaceStack&) = delete;

    void emit_change_notification();

    std::recursive_mutex guard;
    std::shared_ptr<SurfaceFactory> const surface_factory;
    std::shared_ptr<InputRegistrar> const input_registrar;

    typedef std::vector<std::shared_ptr<Surface>> Layer;
    std::map<DepthId, Layer> layers_by_depth;

    std::mutex notify_change_mutex;
    std::function<void()> notify_change;
};

}
}

#endif /* MIR_SURFACES_SURFACESTACK_H_ */
