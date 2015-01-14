/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_EXAMPLES_WINDOW_MANAGEMENT_H_
#define MIR_EXAMPLES_WINDOW_MANAGEMENT_H_

#include "mir/geometry/rectangles.h"
#include "mir/scene/session.h"
#include "mir/scene/surface.h"
#include "mir/scene/placement_strategy.h"
#include "mir/scene/surface_configurator.h"
#include "mir/scene/surface_creation_parameters.h"

#include <memory>

///\example server_example_window_management.h
/// Demonstrate simple window management strategies

namespace mir
{
class Server;

namespace examples
{
class WindowManager :
    public virtual scene::PlacementStrategy,
    public virtual scene::SurfaceConfigurator
{
public:

    virtual void add_surface(
        std::shared_ptr<scene::Surface> const& surface,
        scene::Session* session) = 0;

    virtual void remove_surface(
        std::weak_ptr<scene::Surface> const& surface,
        scene::Session* session) = 0;

    virtual void add_session(std::shared_ptr<scene::Session> const& session) = 0;

    virtual void remove_session(std::shared_ptr<scene::Session> const& session) = 0;

    virtual void add_display(geometry::Rectangle const& area) = 0;

    virtual void remove_display(geometry::Rectangle const& area) = 0;

    virtual void click(geometry::Point cursor) = 0;

    virtual void drag(geometry::Point cursor) = 0;

    virtual void resize(geometry::Point cursor) = 0;

    virtual void toggle_maximized() = 0;

    virtual void toggle_max_horizontal() = 0;

    virtual void toggle_max_vertical() = 0;
};

class EventTracker;

class WindowManagmentFactory
{
public:
    explicit WindowManagmentFactory(Server& server) : server{server} {}

    auto window_manager() -> std::shared_ptr<WindowManager>;

private:
    Server& server;
    std::weak_ptr<WindowManager> wm;
    std::shared_ptr<EventTracker> et;
};

extern char const* const wm_option;
extern char const* const wm_description;
}
}

#endif
