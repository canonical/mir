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

#ifndef MIR_SHELL_MAGNIFICATION_MANAGER_H
#define MIR_SHELL_MAGNIFICATION_MANAGER_H

#include <memory>

namespace mir
{
namespace input
{
class CompositeEventFilter;
class Scene;
}
namespace graphics
{
class GraphicBufferAllocator;
class Cursor;
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

class MagnificationManager
{
public:
    virtual ~MagnificationManager() = default;
    virtual void magnification(float magnification) = 0;
    virtual void enabled(bool enabled) = 0;
};

class BasicMagnificationManager : public MagnificationManager
{
public:
    BasicMagnificationManager(
        std::shared_ptr<input::CompositeEventFilter> const& filter,
        std::shared_ptr<input::Scene> const& scene,
        std::shared_ptr<graphics::GraphicBufferAllocator> const& allocator,
        std::shared_ptr<compositor::ScreenShooter> const& screen_shooter,
        std::shared_ptr<frontend::SurfaceStack> const& surface_stack,
        std::shared_ptr<graphics::Cursor> const& cursor);
    void enabled(bool enabled) override;
    void magnification(float magnification) override;

private:
    class Self;
    std::shared_ptr<Self> self;
};

}
}

#endif //MIR_SHELL_MAGNIFICATION_MANAGER_H
