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

namespace mir_test_framework
{

struct ObservantShell : mir::shell::Shell
{
    ObservantShell(
        std::shared_ptr<mir::shell::Shell> const& wrapped,
        std::shared_ptr<mir::scene::SurfaceObserver> const& surface_observer);

    void add_display(mir::geometry::Rectangle const& area) override;

    void remove_display(mir::geometry::Rectangle const& area) override;

    bool handle(MirEvent const& event) override;

    void focus_next_session() override;

    auto focused_session() const -> std::shared_ptr<mir::scene::Session> override;

    void set_focus_to(
        std::shared_ptr<mir::scene::Session> const& focus_session,
        std::shared_ptr<mir::scene::Surface> const& focus_surface) override;

    std::shared_ptr<mir::scene::Surface> focused_surface() const override;

    auto surface_at(mir::geometry::Point cursor) const -> std::shared_ptr<mir::scene::Surface> override;

    void raise(mir::shell::SurfaceSet const& surfaces) override;

    std::shared_ptr<mir::scene::Session> open_session(
        pid_t client_pid,
        std::string const& name,
        std::shared_ptr<mir::frontend::EventSink> const& sink) override;

    void close_session(std::shared_ptr<mir::scene::Session> const& session) override;

    std::shared_ptr<mir::scene::PromptSession> start_prompt_session_for(
        std::shared_ptr<mir::scene::Session> const& session,
        mir::scene::PromptSessionCreationParameters const& params) override;

    void add_prompt_provider_for(
        std::shared_ptr<mir::scene::PromptSession> const& prompt_session,
        std::shared_ptr<mir::scene::Session> const& session) override;

    void stop_prompt_session(std::shared_ptr<mir::scene::PromptSession> const& prompt_session) override;

    mir::frontend::SurfaceId create_surface(
        std::shared_ptr<mir::scene::Session> const& session,
        mir::scene::SurfaceCreationParameters const& params,
        std::shared_ptr<mir::frontend::EventSink> const& sink) override;

    void modify_surface(
        std::shared_ptr<mir::scene::Session> const& session,
        std::shared_ptr<mir::scene::Surface> const& window,
        mir::shell::SurfaceSpecification  const& modifications) override;

    void destroy_surface(std::shared_ptr<mir::scene::Session> const& session, mir::frontend::SurfaceId window) override;

    int set_surface_attribute(
        std::shared_ptr<mir::scene::Session> const& session,
        std::shared_ptr<mir::scene::Surface> const& window,
        MirWindowAttrib attrib,
        int value) override;

    int get_surface_attribute(
        std::shared_ptr<mir::scene::Surface> const& window,
        MirWindowAttrib attrib) override;

    void raise_surface(
        std::shared_ptr<mir::scene::Session> const& session,
        std::shared_ptr<mir::scene::Surface> const& window,
        uint64_t timestamp) override;

private:
    std::shared_ptr<mir::shell::Shell> const wrapped;
    std::shared_ptr<mir::scene::SurfaceObserver> const surface_observer;
};

}

#endif /* MIR_TEST_OBSERVANT_SHELL_H_ */
