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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_SHELL_SESSION_H_
#define MIR_TEST_DOUBLES_MOCK_SHELL_SESSION_H_

#include "mir/shell/session.h"
#include "mir/shell/surface_creation_parameters.h"
#include "mir/graphics/display_configuration.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

struct MockShellSession : public shell::Session
{
    MOCK_METHOD1(create_surface, frontend::SurfaceId(shell::SurfaceCreationParameters const&));
    MOCK_METHOD1(destroy_surface, void(frontend::SurfaceId));
    MOCK_CONST_METHOD1(get_surface, std::shared_ptr<frontend::Surface>(frontend::SurfaceId));

    MOCK_METHOD1(take_snapshot, void(shell::SnapshotCallback const&));
    MOCK_CONST_METHOD0(default_surface, std::shared_ptr<shell::Surface>());

    MOCK_CONST_METHOD0(name, std::string());
    MOCK_METHOD0(force_requests_to_complete, void());

    MOCK_METHOD0(hide, void());
    MOCK_METHOD0(show, void());
    
    MOCK_METHOD1(send_display_config, void(graphics::DisplayConfiguration const&));
    MOCK_METHOD3(configure_surface, int(frontend::SurfaceId, MirSurfaceAttrib, int));

    MOCK_METHOD1(set_lifecycle_state, void(MirLifecycleState state));
};

}
}
} // namespace mir

#endif // MIR_TEST_DOUBLES_MOCK_SESSION_H_
