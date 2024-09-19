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
#include <memory>
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

    struct WindowSurfaceObserver : public mir::scene::NullSurfaceObserver
    {
    public:
        WindowSurfaceObserver(Decoration* const& decoration) :
            decoration{decoration}
        {
        }

        /// Overrides from NullSurfaceObserver
        /// @{
        void attrib_changed(mir::scene::Surface const* window_surface, MirWindowAttrib attrib, int value) override
        {
            decoration->attrib_changed(window_surface, attrib, value);
        }

        void window_resized_to(mir::scene::Surface const* window_surface, geometry::Size const& window_size) override
        {
            decoration->window_resized_to(window_surface, window_size);
        }

        void renamed(mir::scene::Surface const* window_surface, std::string const& name) override
        {
            decoration->window_renamed(window_surface, name);
        }
        /// @}

        Decoration* const decoration;
    };

    Decoration() = default; // For tests

    Decoration(std::shared_ptr<mir::scene::Surface> decoration_surface, std::shared_ptr<mir::scene::Surface> window_surface) :
        decoration_surface{decoration_surface},
        window_surface{window_surface},
        input_observer{std::make_shared<InputObserver>(this)},
        window_observer{std::make_shared<WindowSurfaceObserver>(this)}
    {
        decoration_surface->register_interest(input_observer);
        window_surface->register_interest(window_observer);
    }

    virtual ~Decoration()
    {
        decoration_surface->unregister_interest(*input_observer);
        window_surface->unregister_interest(*window_observer);
    }

    // Called whenever the visible aspect of the window changes.
    // Examples: Scale, title, focus, visibility, size, etc...
    virtual void redraw() = 0;

    // Called whenever the decoration surface receives an input
    virtual void handle_input_event(std::shared_ptr<MirEvent const> const& /*event*/) = 0;

    virtual void set_scale(float new_scale) = 0;
    virtual void attrib_changed(mir::scene::Surface const* window_surface, MirWindowAttrib attrib, int value) = 0;
    virtual void window_resized_to(mir::scene::Surface const* window_surface, geometry::Size const& window_size) = 0;
    virtual void window_renamed(mir::scene::Surface const* window_surface, std::string const& name) = 0;

private:
    Decoration(Decoration const&) = delete;
    Decoration& operator=(Decoration const&) = delete;

protected:
    std::shared_ptr<mir::scene::Surface> decoration_surface;
    std::shared_ptr<mir::scene::Surface> window_surface;
    std::shared_ptr<InputObserver> input_observer;
    std::shared_ptr<WindowSurfaceObserver> window_observer;
};
}
}
}

#endif // MIR_SHELL_DECORATION_DECORATION_H_
