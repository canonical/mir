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

#ifndef MIR_SHELL_DECORATION_WINDOW_H_
#define MIR_SHELL_DECORATION_WINDOW_H_

#include "mir/geometry/rectangle.h"
#include "mir/geometry/displacement.h"

#include <mir_toolkit/common.h>

#include <memory>
#include <functional>
#include <string>

namespace mir
{
namespace scene
{
class Surface;
}
namespace shell
{
namespace decoration
{
class BasicDecoration;
template<typename T> class ThreadsafeAccess;

/// Observes the decorated window and calls on_update when its state changes
class WindowSurfaceObserverManager
{
public:
    WindowSurfaceObserverManager(
        std::shared_ptr<scene::Surface> const& window_surface,
        std::shared_ptr<ThreadsafeAccess<BasicDecoration>> const& decoration);
    ~WindowSurfaceObserverManager();

private:
    WindowSurfaceObserverManager(WindowSurfaceObserverManager const&) = delete;
    WindowSurfaceObserverManager& operator=(WindowSurfaceObserverManager const&) = delete;

    class Observer;

    std::shared_ptr<scene::Surface> const surface_;
    std::shared_ptr<Observer> const observer;
};
}
}
}

#endif // MIR_SHELL_DECORATION_WINDOW_H_
