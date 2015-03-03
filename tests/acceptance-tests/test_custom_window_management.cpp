/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/shell/generic_shell.h"

#include "mir/geometry/rectangle.h"

#include "mir_test_framework/headless_test.h"
#include "mir_test_doubles/mock_window_manager.h"

#include "mir_test/fake_shared.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace msh = mir::shell;
namespace geom = mir::geometry;

namespace mt = mir::test;
namespace mtd = mir::test::doubles;
namespace mtf = mir_test_framework;
using namespace testing;

namespace
{
std::vector<geom::Rectangle> const display_geometry
{
    {{  0, 0}, { 640,  480}},
    {{480, 0}, {1920, 1080}}
};


using NiceMockWindowManager = NiceMock<mtd::MockWindowManager>;

struct CustomWindowManagement : mtf::HeadlessTest
{
    void SetUp() override
    {
        add_to_environment("MIR_SERVER_NO_FILE", "");

        initial_display_layout(display_geometry);

        server.override_the_shell([this]
           {
                return std::make_shared<msh::GenericShell>(
                    server.the_input_targeter(),
                    server.the_surface_coordinator(),
                    server.the_session_coordinator(),
                    server.the_prompt_session_manager(),
                    [this](msh::FocusController*) { return mt::fake_shared(window_manager); });
           });
    }

    void TearDown() override
    {
        stop_server();
    }

    NiceMockWindowManager window_manager;
};
}

TEST_F(CustomWindowManagement, display_layout_is_notified_on_startup)
{
    for(auto const& rect : display_geometry)
        EXPECT_CALL(window_manager, add_display(rect));

    start_server();
}

TEST_F(CustomWindowManagement, display_layout_is_notified_on_shutdown)
{
    start_server();

    for(auto const& rect : display_geometry)
        EXPECT_CALL(window_manager, remove_display(rect));
}
