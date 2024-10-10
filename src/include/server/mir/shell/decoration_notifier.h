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

#include "mir/shell/shell.h"
#include "mir_toolkit/common.h"

#include "mir/scene/null_surface_observer.h"
#include "mir/scene/surface.h"
#include <memory>
#include <mir/geometry/size.h>
#include <mir/geometry/forward.h>
#include <mir/events/event.h>

#include <string>

namespace mir::shell::decoration
{
class DecorationNotifier
{
public:
    DecorationNotifier(
        std::shared_ptr<mir::scene::Surface> decoration_surface,
        std::shared_ptr<mir::scene::Surface> window_surface,
        Decoration* const decoration) :
        decoration_surface{decoration_surface},
        window_surface{window_surface},
        input_observer{std::make_shared<InputObserver>(decoration)},
        window_observer{std::make_shared<WindowSurfaceObserver>(decoration)}
    {
        decoration_surface->register_interest(input_observer);
        window_surface->register_interest(window_observer);
    }

    ~DecorationNotifier()
    {
        if (decoration_surface)
            decoration_surface->unregister_interest(*input_observer);

        if (window_surface)
            window_surface->unregister_interest(*window_observer);
    }

private:
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

    std::shared_ptr<mir::scene::Surface> decoration_surface;
    std::shared_ptr<mir::scene::Surface> window_surface;
    std::shared_ptr<InputObserver> input_observer;
    std::shared_ptr<WindowSurfaceObserver> window_observer;
};
}
