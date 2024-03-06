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

    MOCK_METHOD(void, starting, (std::shared_ptr<scene::Session> const&), (override));
    MOCK_METHOD(void, stopping, (std::shared_ptr<scene::Session> const&), (override));
    MOCK_METHOD(void, focused, (std::shared_ptr<scene::Session> const&), (override));
    MOCK_METHOD(void, unfocused, (), (override));

    MOCK_METHOD(void, surface_created, (scene::Session&, std::shared_ptr<scene::Surface> const&), (override));
    MOCK_METHOD(void, destroying_surface, (scene::Session&, std::shared_ptr<scene::Surface> const&), (override));

    MOCK_METHOD(void, buffer_stream_created, (scene::Session&, std::shared_ptr<frontend::BufferStream> const&), (override));
    MOCK_METHOD(void, buffer_stream_destroyed, (scene::Session&, std::shared_ptr<frontend::BufferStream> const&), (override));
};

}
}
} // namespace mir

#endif // MIR_TEST_DOUBLES_MOCK_SESSION_LISTENER_H_
