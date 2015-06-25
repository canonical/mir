/*
 * Copyright © 2013 Canonical Ltd.
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
    std::string name() const override
    {
        return std::string();
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
};

}
}
} // namespace mir

#endif // MIR_TEST_DOUBLES_STUB_SESSION_H_
