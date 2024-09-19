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

#ifndef MIR_SHELL_DECORATION_DECORATION_H_
#define MIR_SHELL_DECORATION_DECORATION_H_


#include "mir/scene/null_surface_observer.h"
#include "mir/scene/surface.h"
#include <mir/geometry/size.h>
#include <mir/geometry/forward.h>
#include <mir/events/event.h>

#include <string>

namespace mir
{
namespace shell
{
namespace decoration
{
/// Draws window decorations and provides basic move, resize, close, etc functionality
class Decoration
{
public:
    struct InputObserver : public mir::scene::NullSurfaceObserver
    {
        InputObserver(Decoration* decoration) :
            decoration{decoration}
        {
        }

        /// Overrides from NullSurfaceObserver
        /// @{
        void input_consumed(mir::scene::Surface const*, std::shared_ptr<MirEvent const> const& event) override
        {
            if (mir_event_get_type(event.get()) != mir_event_type_input)
                return;

            decoration->handle_input_event(event);
        }
        /// @}

        Decoration* const decoration;
    };

    Decoration() = default; // For tests

    Decoration(std::shared_ptr<mir::scene::Surface> decoration_surface) :
        decoration_surface{decoration_surface},
        observer{std::make_shared<InputObserver>(this)}
    {
        decoration_surface->register_interest(observer);
    }

    virtual ~Decoration()
    {
        decoration_surface->unregister_interest(*observer);
    }

    // Called whenever the visible aspect of the window changes.
    // Examples: Scale, title, focus, visibility, size, etc...
    virtual void redraw() = 0;

    // Called whenever the input state changes
    // Mouse up, down, dragging, enter, leave...
    virtual void input_state_changed() = 0;

    // Called whenever the decoration surface receives an input
    virtual void handle_input_event(std::shared_ptr<MirEvent const> const& /*event*/) { /* Empty for tests */ }

    // Basic functionality that all decorations _should_ have
    virtual void request_toggle_maximize() = 0;
    virtual void request_close() = 0;
    virtual void request_minimize() = 0;
    virtual void request_move(MirInputEvent const*) = 0;
    virtual void request_resize(MirInputEvent const*, MirResizeEdge) = 0;
    virtual void set_cursor(std::string const& cursor_name) = 0;
    virtual void set_scale(float new_scale) = 0;

private:
    Decoration(Decoration const&) = delete;
    Decoration& operator=(Decoration const&) = delete;

protected:
    std::shared_ptr<mir::scene::Surface> decoration_surface;
    std::shared_ptr<InputObserver> observer;
};
}
}
}

#endif // MIR_SHELL_DECORATION_DECORATION_H_
