/*
 * Copyright © 2017 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Brandon Schaefer <brandon.schaefer@canonical.com>
 */

#ifndef MIR_TEST_OBSERVANT_SHELL_H_
#define MIR_TEST_OBSERVANT_SHELL_H_

#include "mir/scene/session.h"
#include "mir/scene/surface.h"
#include "mir/shell/shell.h"

namespace mir
{
namespace test
{
namespace doubles
{

struct ObservantShell : shell::Shell
{
    ObservantShell(
        std::shared_ptr<shell::Shell> const& wrapped,
        std::shared_ptr<scene::SurfaceObserver> const& surface_observer) :
        wrapped(wrapped),
        surface_observer(surface_observer)
    {
    }

    void add_display(geometry::Rectangle const& area) override
    {
        return wrapped->add_display(area);
    }

    void remove_display(geometry::Rectangle const& area) override
    {
        return wrapped->remove_display(area);
    }

    bool handle(MirEvent const& event) override
    {
        return wrapped->handle(event);
    }

    void focus_next_session() override
    {
        wrapped->focus_next_session();
    }

    auto focused_session() const -> std::shared_ptr<scene::Session> override
    {
        return wrapped->focused_session();
    }

    void set_focus_to(
        std::shared_ptr<scene::Session> const& focus_session,
        std::shared_ptr<scene::Surface> const& focus_surface) override
    {
        return wrapped->set_focus_to(focus_session, focus_surface);
    }

    std::shared_ptr<scene::Surface> focused_surface() const override
    {
        return wrapped->focused_surface();
    }

    auto surface_at(geometry::Point cursor) const -> std::shared_ptr<scene::Surface> override
    {
        return wrapped->surface_at(cursor);
    }

    void raise(shell::SurfaceSet const& surfaces) override
    {
        wrapped->raise(surfaces);
    }

    std::shared_ptr<scene::Session> open_session(
        pid_t client_pid,
        std::string const& name,
        std::shared_ptr<frontend::EventSink> const& sink) override
    {
        return wrapped->open_session(client_pid, name, sink);
    }

    void close_session(std::shared_ptr<scene::Session> const& session) override
    {
        wrapped->close_session(session);
    }

    std::shared_ptr<scene::PromptSession> start_prompt_session_for(
        std::shared_ptr<scene::Session> const& session,
        scene::PromptSessionCreationParameters const& params) override
    {
        return wrapped->start_prompt_session_for(session, params);
    }

    void add_prompt_provider_for(
        std::shared_ptr<scene::PromptSession> const& prompt_session,
        std::shared_ptr<scene::Session> const& session) override
    {
        wrapped->add_prompt_provider_for(prompt_session, session);
    }

    void stop_prompt_session(std::shared_ptr<scene::PromptSession> const& prompt_session) override
    {
        wrapped->stop_prompt_session(prompt_session);
    }

    frontend::SurfaceId create_surface(
        std::shared_ptr<scene::Session> const& session,
        scene::SurfaceCreationParameters const& params,
        std::shared_ptr<frontend::EventSink> const& sink) override
    {
        auto id = wrapped->create_surface(session, params, sink);
        auto window = session->surface(id);
        window->add_observer(surface_observer);
        return id;
    }

    void modify_surface(
        std::shared_ptr<scene::Session> const& session,
        std::shared_ptr<scene::Surface> const& window,
        shell::SurfaceSpecification  const& modifications) override
    {
        wrapped->modify_surface(session, window, modifications);
    }

    void destroy_surface(std::shared_ptr<scene::Session> const& session, frontend::SurfaceId window) override
    {
        wrapped->destroy_surface(session, window);
    }

    int set_surface_attribute(
        std::shared_ptr<scene::Session> const& session,
        std::shared_ptr<scene::Surface> const& window,
        MirWindowAttrib attrib,
        int value) override
    {
        return wrapped->set_surface_attribute(session, window, attrib, value);
    }

    int get_surface_attribute(
        std::shared_ptr<scene::Surface> const& window,
        MirWindowAttrib attrib) override
    {
        return wrapped->get_surface_attribute(window, attrib);
    }

    void raise_surface(
        std::shared_ptr<scene::Session> const& session,
        std::shared_ptr<scene::Surface> const& window,
        uint64_t timestamp) override
    {
        return wrapped->raise_surface(session, window, timestamp);
    }

private:
    std::shared_ptr<shell::Shell> const wrapped;
    std::shared_ptr<scene::SurfaceObserver> const surface_observer;
};

}
}
}

#endif /* MIR_TEST_OBSERVANT_SHELL_H_ */
