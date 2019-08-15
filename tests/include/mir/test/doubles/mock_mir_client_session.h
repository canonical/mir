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

#ifndef MIR_TEST_DOUBLES_MOCK_SESSION_H_
#define MIR_TEST_DOUBLES_MOCK_SESSION_H_

#include "mir/frontend/mir_client_session.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

struct MockMirClientSession : public frontend::MirClientSession
{
    MOCK_CONST_METHOD1(get_surface, std::shared_ptr<frontend::Surface>(frontend::SurfaceId));

    MOCK_CONST_METHOD1(get_buffer_stream, std::shared_ptr<frontend::BufferStream>(frontend::BufferStreamId));
    MOCK_METHOD1(create_buffer_stream, frontend::BufferStreamId(graphics::BufferProperties const&));
    MOCK_METHOD1(destroy_buffer_stream, void(frontend::BufferStreamId));

    MOCK_CONST_METHOD0(name, std::string());
};

}
}
} // namespace mir


#endif // MIR_TEST_DOUBLES_MOCK_SESSION_H_
