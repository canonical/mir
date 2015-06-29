/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_SCREENCAST_H_
#define MIR_TEST_DOUBLES_MOCK_SCREENCAST_H_

#include "mir/frontend/screencast.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

class MockScreencast : public frontend::Screencast
{
public:
    MOCK_METHOD3(create_session,
                 frontend::ScreencastSessionId(
                     geometry::Rectangle const&,
                     geometry::Size const&,
                     MirPixelFormat));
    MOCK_METHOD1(destroy_session, void(frontend::ScreencastSessionId));
    MOCK_METHOD1(capture,
                 std::shared_ptr<graphics::Buffer>(
                     frontend::ScreencastSessionId));
};

}
}
}

#endif /* MIR_TEST_DOUBLES_NULL_SCREENCAST_H_ */

