/*
 * Copyright © 2013 Canonical Ltd.
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
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_STUB_SESSION_H_
#define MIR_TEST_DOUBLES_STUB_SESSION_H_

#include "mir/frontend/session.h"

namespace mir
{
namespace test
{
namespace doubles
{

struct StubSession : public frontend::Session
{
    std::shared_ptr<frontend::Surface> get_surface(frontend::SurfaceId /* surface */) const override
    {
        return std::shared_ptr<frontend::Surface>();
    }

    auto process_id() const -> pid_t override
    {
        return 0;
    }

    auto name() const -> std::string override
    {
        return "";
    }

    void send_display_config(graphics::DisplayConfiguration const&) override
    {
    }

    void send_error(ClientVisibleError const& /* error */) override
    {
    }

    void send_input_config(MirInputConfig const& /* config */) override
    {
    }

    void take_snapshot(scene::SnapshotCallback const& /* snapshot_taken */) override
    {
    }

    auto default_surface() const -> std::shared_ptr<scene::Surface> override
    {
        return nullptr;
    }

    void set_lifecycle_state(MirLifecycleState /* state */) override
    {
    }

    void hide() override
    {
    }

    void show() override
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

    auto create_surface(
        scene::SurfaceCreationParameters const& /* params */,
        std::shared_ptr<frontend::EventSink> const& /* sink */) -> frontend::SurfaceId override
    {
        return {};
    }

    void destroy_surface(frontend::SurfaceId /* surface */) override
    {
    }

    auto surface(frontend::SurfaceId /* surface */) const -> std::shared_ptr<scene::Surface> override
    {
        return nullptr;
    }

    void destroy_surface(std::weak_ptr<scene::Surface> const& /* surface */) override
    {
    }

    auto surface_after(std::shared_ptr<scene::Surface> const& /* surface */) const
        -> std::shared_ptr<scene::Surface> override
    {
        return nullptr;
    }

    auto create_buffer_stream(graphics::BufferProperties const& /* props */)
        -> frontend::BufferStreamId override
    {
        return {};
    }

    auto get_buffer_stream(frontend::BufferStreamId /* stream */) const
        -> std::shared_ptr<frontend::BufferStream> override
    {
        return nullptr;
    }

    void destroy_buffer_stream(frontend::BufferStreamId /* stream */) override
    {
    }

    void configure_streams(
        scene::Surface& /* surface */,
        std::vector<shell::StreamSpecification> const& /* config */) override
    {
    }
};

}
}
} // namespace mir

#endif // MIR_TEST_DOUBLES_STUB_SESSION_H_
