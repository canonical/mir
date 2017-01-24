/*
 * Copyright © 2013-2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
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

#include "mir/scene/surface.h"

#include "mir_test_framework/connected_client_with_a_surface.h"

#include "mir/shell/canonical_window_manager.h"

#include "mir_toolkit/mir_client_library.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace ms = mir::scene;
namespace msh = mir::shell;

namespace mtf = mir_test_framework;
using namespace ::testing;

namespace
{
struct MockWindowManager : msh::CanonicalWindowManager
{
    using msh::CanonicalWindowManager::CanonicalWindowManager;

    MOCK_METHOD4(set_surface_attribute,
        int(std::shared_ptr<ms::Session> const& session,
            std::shared_ptr<ms::Surface> const& surface,
            MirWindowAttrib attrib,
            int value));

    int real_set_surface_attribute(std::shared_ptr<ms::Session> const& session,
                std::shared_ptr<ms::Surface> const& surface,
                MirWindowAttrib attrib,
                int value)
    {
        return msh::CanonicalWindowManager::set_surface_attribute(session, surface, attrib, value);
    }
};

struct ShellSurfaceConfiguration : mtf::ConnectedClientWithASurface
{
    void SetUp() override
    {
        server.override_the_window_manager_builder([this]
            (msh::FocusController* focus_controller) -> std::shared_ptr<msh::WindowManager>
            {
                mock_window_manager = std::make_shared<MockWindowManager>(
                    focus_controller,
                    server.the_shell_display_layout());

                ON_CALL(*mock_window_manager, set_surface_attribute(_, _, _, _))
                    .WillByDefault(Invoke(
                        mock_window_manager.get(), &MockWindowManager::real_set_surface_attribute));

                EXPECT_CALL(*mock_window_manager,
                    set_surface_attribute(_, _, Ne(mir_window_attrib_state), _))
                    .Times(AnyNumber());

                return mock_window_manager;
            });

        mtf::ConnectedClientWithASurface::SetUp();
    }

    std::shared_ptr<MockWindowManager> mock_window_manager;
};
}

TEST_F(ShellSurfaceConfiguration, the_window_manager_is_notified_of_attribute_changes)
{
    EXPECT_CALL(*mock_window_manager,
        set_surface_attribute(_, _, mir_window_attrib_state, Eq(mir_window_state_maximized)));

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    mir_wait_for(mir_window_set_state(window, mir_window_state_maximized));
#pragma GCC diagnostic pop

    EXPECT_THAT(mir_window_get_state(window), Eq(mir_window_state_maximized));
}

TEST_F(ShellSurfaceConfiguration, the_window_manager_may_interfere_with_attribute_changes)
{
    auto const set_to_vertmax = [this](
        std::shared_ptr<ms::Session> const& session,
        std::shared_ptr<ms::Surface> const& surface,
        MirWindowAttrib attrib,
        int /*value*/)
    {
        return mock_window_manager->real_set_surface_attribute(
            session, surface, attrib, mir_window_state_vertmaximized);
    };

    EXPECT_CALL(*mock_window_manager,
        set_surface_attribute(_, _, mir_window_attrib_state, Eq(mir_window_state_maximized)))
        .WillOnce(Invoke(set_to_vertmax));

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    mir_wait_for(mir_window_set_state(window, mir_window_state_maximized));
#pragma GCC diagnostic pop

    EXPECT_THAT(mir_window_get_state(window), Eq(mir_window_state_vertmaximized));
}
