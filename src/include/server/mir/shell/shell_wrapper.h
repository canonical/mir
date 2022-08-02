/*
 * Copyright © 2015-2019 Canonical Ltd.
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

#ifndef MIR_SHELL_SHELL_WRAPPER_H_
#define MIR_SHELL_SHELL_WRAPPER_H_

#include "mir/shell/shell.h"

namespace mir
{
namespace shell
{
class ShellWrapper : public Shell
{
public:
    explicit ShellWrapper(std::shared_ptr<Shell> const& wrapped);

    void focus_next_session() override;
    void focus_prev_session() override;

    auto focused_session() const -> std::shared_ptr<scene::Session> override;

    void set_focus_to(
        std::shared_ptr<scene::Session> const& focus_session,
        std::shared_ptr<scene::Surface> const& focus_surface) override;

    auto focused_surface() const -> std::shared_ptr<scene::Surface> override;

    auto surface_at(geometry::Point cursor) const -> std::shared_ptr<scene::Surface> override;

    void raise(SurfaceSet const& surfaces) override;

    auto open_session(
        pid_t client_pid,
        Fd socket_fd,
        std::string const& name,
        std::shared_ptr<frontend::EventSink> const& sink) -> std::shared_ptr<scene::Session> override;

    void close_session(std::shared_ptr<scene::Session> const& session) override;

    auto start_prompt_session_for(
        std::shared_ptr<scene::Session> const& session,
        scene::PromptSessionCreationParameters const& params) -> std::shared_ptr<scene::PromptSession> override;

    void add_prompt_provider_for(
        std::shared_ptr<scene::PromptSession> const& prompt_session,
        std::shared_ptr<scene::Session> const& session) override;

    void stop_prompt_session(std::shared_ptr<scene::PromptSession> const& prompt_session) override;

    auto create_surface(
        std::shared_ptr<scene::Session> const& session,
        wayland::Weak<frontend::WlSurface> const& wayland_surface,
        SurfaceSpecification const& params,
        std::shared_ptr<scene::SurfaceObserver> const& observer) -> std::shared_ptr<scene::Surface> override;

    void surface_ready(std::shared_ptr<scene::Surface> const& surface) override;

    void modify_surface(
        std::shared_ptr<scene::Session> const& session,
        std::shared_ptr<scene::Surface> const& surface,
        SurfaceSpecification const& modifications) override;

    void destroy_surface(
        std::shared_ptr<scene::Session> const& session,
        std::shared_ptr<scene::Surface> const& surface) override;

    int set_surface_attribute(
        std::shared_ptr<scene::Session> const& session,
        std::shared_ptr<scene::Surface> const& surface,
        MirWindowAttrib attrib,
        int value) override;

    int get_surface_attribute(
        std::shared_ptr<scene::Surface> const& surface,
        MirWindowAttrib attrib) override;

    void raise_surface(
        std::shared_ptr<scene::Session> const& session,
        std::shared_ptr<scene::Surface> const& surface,
        uint64_t timestamp) override;

    void request_drag_and_drop(
        std::shared_ptr<scene::Session> const& session,
        std::shared_ptr<scene::Surface> const& surface,
        uint64_t timestamp) override;

    void request_move(
        std::shared_ptr<scene::Session> const& session,
        std::shared_ptr<scene::Surface> const& surface,
        uint64_t timestamp) override;

    void request_resize(
        std::shared_ptr<scene::Session> const& session,
        std::shared_ptr<scene::Surface> const& surface,
        uint64_t timestamp,
        MirResizeEdge edge) override;

    void add_display(geometry::Rectangle const& area) override;
    void remove_display(geometry::Rectangle const& area) override;

    bool handle(MirEvent const& event) override;

    void set_drag_and_drop_handle(std::vector<uint8_t> const& handle) override;
    void clear_drag_and_drop_handle() override;

protected:
    std::shared_ptr<Shell> const wrapped;
};
}
}

#endif /* MIR_SHELL_SHELL_WRAPPER_H_ */
