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

#ifndef MIR_SHELL_BASIC_MAGNIFICATION_MANAGER_H
#define MIR_SHELL_BASIC_MAGNIFICATION_MANAGER_H

#include "mir/shell/magnification_manager.h"
#include <memory>

namespace mir
{
namespace input
{
class CompositeEventFilter;
class Scene;
class Seat;
}
namespace graphics
{
class GraphicBufferAllocator;
}
namespace compositor
{
class ScreenShooter;
}
namespace frontend
{
class SurfaceStack;
}

namespace shell
{

class BasicMagnificationManager : public MagnificationManager
{
public:
    BasicMagnificationManager(
        std::shared_ptr<input::CompositeEventFilter> const& filter,
        std::shared_ptr<input::Scene> const& scene,
        std::shared_ptr<graphics::GraphicBufferAllocator> const& allocator,
        std::shared_ptr<compositor::ScreenShooter> const& screen_shooter,
        std::shared_ptr<frontend::SurfaceStack> const& surface_stack,
        std::shared_ptr<input::Seat> const& seat);
    bool enabled(bool enabled) override;
    void magnification(float magnification) override;
    float magnification() const override;
    void size(geometry::Size const& size) override;
    geometry::Size size() const override;

private:
    class Self;
    std::shared_ptr<Self> self;
};

}
}

#endif //MIR_SHELL_BASIC_MAGNIFICATION_MANAGER_H
