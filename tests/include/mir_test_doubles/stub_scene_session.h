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

#ifndef MIR_TEST_DOUBLES_STUB_SCENE_SESSION_H_
#define MIR_TEST_DOUBLES_STUB_SCENE_SESSION_H_

#include "mir/scene/session.h"

namespace mir
{
namespace test
{
namespace doubles
{

struct StubSceneSession : public scene::Session
{
    StubSceneSession(pid_t pid = -1) : pid(pid) {}

    frontend::SurfaceId create_surface(scene::SurfaceCreationParameters const& /* params */) override
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
    std::shared_ptr<scene::Surface> surface(frontend::SurfaceId /* surface */) const override
    {
        return std::shared_ptr<scene::Surface>();
    }
    std::shared_ptr<scene::Surface> surface_after(std::shared_ptr<scene::Surface> const& s) const override
    {
        return s;
    }
    std::string name() const override
    {
        return std::string();
    }

    pid_t process_id() const override
    {
        return pid;
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

    void send_display_config(graphics::DisplayConfiguration const&) override
    {
    }

    void take_snapshot(scene::SnapshotCallback const&) override
    {
    }

    std::shared_ptr<scene::Surface> default_surface() const override
    {
        return std::shared_ptr<scene::Surface>();
    }

    void set_lifecycle_state(MirLifecycleState /*state*/)
    {
    }

    void start_prompt_session() override
    {
    }

    void stop_prompt_session() override
    {
    }

    void suspend_prompt_session() override
    {
    }

    void resume_prompt_session() override
    {
    }

    std::shared_ptr<frontend::BufferStream> get_buffer_stream(frontend::BufferStreamId) const override
    {
        return nullptr;
    }
    void destroy_buffer_stream(frontend::BufferStreamId) override
    {
    }
    
    frontend::BufferStreamId create_buffer_stream(graphics::BufferProperties const&) override
    {
        return frontend::BufferStreamId();
    }

    void configure_streams(scene::Surface&, std::vector<shell::StreamSpecification> const&)
    {
    }

    pid_t const pid;
};

}
}
} // namespace mir

#endif // MIR_TEST_DOUBLES_STUB_SCENE_SESSION_H_

