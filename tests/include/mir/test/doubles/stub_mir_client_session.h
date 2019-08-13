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

#include "mir/frontend/mir_client_session.h"

namespace mir
{
namespace test
{
namespace doubles
{

struct StubMirClientSession : public frontend::MirClientSession
{
    auto name() const -> std::string override
    {
        return "";
    }

    auto get_surface(frontend::SurfaceId /* surface */) const -> std::shared_ptr<frontend::Surface> override
    {
        return nullptr;
    }

    auto create_surface(
        std::shared_ptr<shell::Shell> const& /* shell */,
        scene::SurfaceCreationParameters const& /* params */,
        std::shared_ptr<frontend::EventSink> const& /* sink */) -> frontend::SurfaceId override
    {
        return {};
    }

    void destroy_surface(std::shared_ptr<shell::Shell> const& /* shell */, frontend::SurfaceId /* surface */) override
    {
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
};

}
}
} // namespace mir

#endif // MIR_TEST_DOUBLES_STUB_SESSION_H_
