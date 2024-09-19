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

#include <mir/shell/decoration.h>

#include "mir/scene/surface.h"
#include "mir/shell/shell.h"
#include <mir/events/event.h>
#include <mir/geometry/forward.h>
#include <mir/geometry/size.h>

#include <memory>


namespace mir::shell::decoration
{
class DecorationNotifier
{
public:
    DecorationNotifier(
        std::shared_ptr<mir::scene::Surface> decoration_surface,
        std::shared_ptr<mir::scene::Surface> window_surface,
        Decoration* const decoration);

    ~DecorationNotifier();

private:
    struct InputObserver;
    struct WindowSurfaceObserver;

    std::shared_ptr<mir::scene::Surface> decoration_surface;
    std::shared_ptr<mir::scene::Surface> window_surface;
    std::shared_ptr<InputObserver> input_observer;
    std::shared_ptr<WindowSurfaceObserver> window_observer;
};
}
