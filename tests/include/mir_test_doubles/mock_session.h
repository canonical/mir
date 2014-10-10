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

#ifndef MIR_TEST_DOUBLES_MOCK_SESSION_H_
#define MIR_TEST_DOUBLES_MOCK_SESSION_H_

#include "mir/frontend/session.h"
#include "mir/scene/surface_creation_parameters.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

struct MockSession : public frontend::Session
{
    MOCK_METHOD1(create_surface, frontend::SurfaceId(scene::SurfaceCreationParameters const&));
    MOCK_METHOD1(destroy_surface, void(frontend::SurfaceId));
    MOCK_CONST_METHOD1(get_surface, std::shared_ptr<frontend::Surface>(frontend::SurfaceId));

    MOCK_CONST_METHOD0(name, std::string());
    MOCK_METHOD0(force_requests_to_complete, void());

    MOCK_METHOD0(hide, void());
    MOCK_METHOD0(show, void());

    MOCK_METHOD1(send_display_config, void(graphics::DisplayConfiguration const&));
    MOCK_METHOD3(configure_surface, int(frontend::SurfaceId, MirSurfaceAttrib, int));
};

}
}
} // namespace mir


#endif // MIR_TEST_DOUBLES_MOCK_SESSION_H_
