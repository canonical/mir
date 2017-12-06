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

#ifndef MIR_TEST_DOUBLES_MOCK_SESSION_LISTENER_H_
#define MIR_TEST_DOUBLES_MOCK_SESSION_LISTENER_H_

#include "mir/scene/session_listener.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

struct MockSessionListener : public scene::SessionListener
{
    virtual ~MockSessionListener() noexcept(true) {}

    MOCK_METHOD1(starting, void(std::shared_ptr<scene::Session> const&));
    MOCK_METHOD1(stopping, void(std::shared_ptr<scene::Session> const&));
    MOCK_METHOD1(focused, void(std::shared_ptr<scene::Session> const&));
    MOCK_METHOD0(unfocused, void());

    MOCK_METHOD2(surface_created, void(scene::Session&, std::shared_ptr<scene::Surface> const&));
    MOCK_METHOD2(destroying_surface, void(scene::Session&, std::shared_ptr<scene::Surface> const&));

    MOCK_METHOD2(buffer_stream_created, void(scene::Session&, std::shared_ptr<frontend::BufferStream> const&));
    MOCK_METHOD2(buffer_stream_destroyed, void(scene::Session&, std::shared_ptr<frontend::BufferStream> const&));
};

}
}
} // namespace mir

#endif // MIR_TEST_DOUBLES_MOCK_SESSION_LISTENER_H_
