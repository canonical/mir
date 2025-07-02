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

#ifndef MIR_TEST_DOUBLES_STUB_SESSION_H
#define MIR_TEST_DOUBLES_STUB_SESSION_H

#include <mir/scene/session.h>

namespace mir
{
namespace test
{
namespace doubles
{

struct StubSession : scene::Session
{
    StubSession(pid_t pid = -1);

    std::string name() const override;

    pid_t process_id() const override;

    Fd socket_fd() const override;

    std::shared_ptr<scene::Surface> default_surface() const override;

    void send_error(ClientVisibleError const&) override;

    void hide() override;

    void show() override;

    void start_prompt_session() override;

    void stop_prompt_session() override;

    void suspend_prompt_session() override;

    void resume_prompt_session() override;

    auto create_surface(
        std::shared_ptr<Session> const& session,
        wayland::Weak<frontend::WlSurface> const& wayland_surface,
        shell::SurfaceSpecification const& params,
        std::shared_ptr<scene::SurfaceObserver> const& observer,
        Executor* observer_executor) -> std::shared_ptr<scene::Surface> override;

    void destroy_surface(std::shared_ptr<scene::Surface> const& surface) override;

    auto surface_after(
        std::shared_ptr<scene::Surface> const&) const -> std::shared_ptr<scene::Surface> override;

    auto create_buffer_stream(
        graphics::BufferProperties const& props) -> std::shared_ptr<compositor::BufferStream> override;

    void destroy_buffer_stream(std::shared_ptr<frontend::BufferStream> const& stream) override;

    void configure_streams(
        scene::Surface& surface,
        std::vector<shell::StreamSpecification> const& config) override;

    pid_t pid;
};
}
}
}

#endif //MIR_TEST_DOUBLES_STUB_SESSION_H
