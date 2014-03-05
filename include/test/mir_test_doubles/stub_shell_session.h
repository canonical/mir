/*
 * Copyright © 2013-2014 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_STUB_SHELL_SESSION_H_
#define MIR_TEST_DOUBLES_STUB_SHELL_SESSION_H_

#include "mir/shell/session.h"
#include "src/server/scene/default_session_container.h"
#include <memory>

namespace mir
{
namespace test
{
namespace doubles
{

struct StubShellSession : public shell::Session
{
    StubShellSession():
        children(std::make_shared<scene::DefaultSessionContainer>())
    {}

    frontend::SurfaceId create_surface(shell::SurfaceCreationParameters const& /* params */) override
    {
        return frontend::SurfaceId{0};
    }
    void destroy_surface(frontend::SurfaceId /* surface */) override
    {
    }
    std::shared_ptr<frontend::Surface> get_surface(frontend::SurfaceId /* surface */) const override
    {
        return std::shared_ptr<frontend::Surface>();
    }
    std::string name() const override
    {
        return std::string();
    }
    pid_t process_id() const override
    {
        return -1;
    }

    void force_requests_to_complete() override
    {
    }
    void hide() override
    {
    }
    void show() override
    {
    }
    int configure_surface(frontend::SurfaceId, MirSurfaceAttrib, int) override
    {
        return 0;
    }

    void send_display_config(graphics::DisplayConfiguration const&) override
    {
    }

    void take_snapshot(shell::SnapshotCallback const&) override
    {
    }

    std::shared_ptr<shell::Surface> default_surface() const override
    {
        return std::shared_ptr<shell::Surface>();
    }

    void set_lifecycle_state(MirLifecycleState /*state*/)
    {
    }

    std::shared_ptr<shell::TrustSession> get_trust_session() const override
    {
        return std::shared_ptr<shell::TrustSession>();
    }

    void set_trust_session(std::shared_ptr<shell::TrustSession> const& /* trust_session */) override
    {
    }

    std::shared_ptr<shell::Session> get_parent() const override
    {
        return std::shared_ptr<shell::Session>();
    }

    void set_parent(std::shared_ptr<shell::Session> const& /* new_parent */) override
    {
    }

    std::shared_ptr<scene::SessionContainer> get_children() const override
    {
        return children;
    }

private:
    std::shared_ptr<scene::DefaultSessionContainer> children;

};

}
}
} // namespace mir

#endif // MIR_TEST_DOUBLES_STUB_SHELL_SESSION_H_

