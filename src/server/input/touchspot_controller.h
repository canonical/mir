/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_INPUT_TOUCHSPOT_CONTROLLER_H_
#define MIR_INPUT_TOUCHSPOT_CONTROLLER_H_

#include "mir/input/touch_visualizer.h"

#include <memory>
#include <mutex>

namespace mir
{
namespace graphics
{
class GraphicBufferAllocator;
class Buffer;
class Renderable;
}
namespace input
{
class Scene;
class TouchspotRenderable;

/// Receives touchspot events out of the input stack and manages appearance
/// of touchspot renderables for visualization. Touchspot visualization is
/// disabled by default and must be enabled through a call to ::enable.
class TouchspotController : public TouchVisualizer
{
public:
    TouchspotController(std::shared_ptr<graphics::GraphicBufferAllocator> const& allocator,
        std::shared_ptr<input::Scene> const& scene);

    virtual ~TouchspotController() = default;

    void enable() override;
    void disable() override;

    void visualize_touches(std::vector<Spot> const& touches) override;

protected:
    TouchspotController(TouchspotController const&) = delete;
    TouchspotController& operator=(TouchspotController const&) = delete;

private:
    // Need to keep a reference to the allocator because that keeps the
    // graphics module loaded in memory. Otherwise, the module gets unloaded
    // before `touchspot_buffer` gets deallocated, and it tries to call into it
    // when the application is shutting down, casing a crash. This is a
    // temporary workaround until we fix up how things reference graphics
    // modules.
    //
    // See
    //  https://github.com/canonical/mir/issues/3768
    //  https://github.com/canonical/mir/issues/3779
    std::shared_ptr<graphics::Buffer> const touchspot_buffer;
    std::shared_ptr<Scene> const scene;

    std::mutex guard;

    bool enabled;

    unsigned int renderables_in_use;
    std::vector<std::shared_ptr<TouchspotRenderable>> touchspot_renderables;
};

}
}

#endif // MIR_INPUT_TOUCHSPOT_CONTROLLER_H_
