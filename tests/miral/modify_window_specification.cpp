/*
 * Copyright © Canonical Ltd.
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
 */

#include "test_window_manager_tools.h"
#include <mir/event_printer.h>

using namespace miral;
using namespace testing;
namespace mt = mir::test;
using mir::operator<<;

namespace
{
X const display_left{0};
Y const display_top{0};
Width const display_width{640};
Height const display_height{480};

Rectangle const display_area{{display_left,  display_top},
                             {display_width, display_height}};

struct ModifyWindowState : mt::TestWindowManagerTools, WithParamInterface<MirWindowType>
{
    Size const initial_parent_size{600, 400};

    void SetUp() override
    {
        notify_configuration_applied(create_fake_display_configuration({display_area}));
        basic_window_manager.add_session(session);
    }

    auto create_window_of_type(MirWindowType type, Window const& parent = Window{}) -> Window
    {
        Window window;

        mir::shell::SurfaceSpecification creation_parameters;
        creation_parameters.type = type;
        creation_parameters.parent = parent;
        creation_parameters.set_size(initial_parent_size);

        EXPECT_CALL(*window_manager_policy, advise_new_window(_))
            .WillOnce(
                Invoke(
                    [&window](WindowInfo const& window_info)
                        { window = window_info.window(); }));

        basic_window_manager.add_surface(session, creation_parameters, &create_surface);
        basic_window_manager.select_active_window(window);

        // Clear the expectations used to capture parent & child
        Mock::VerifyAndClearExpectations(window_manager_policy);

        return window;
    }
};
}

TEST_F(ModifyWindowState, can_divorce_parent)
{
    auto const parent = create_window_of_type(mir_window_type_normal);
    auto const child  = create_window_of_type(mir_window_type_freestyle, parent);

    mir::shell::SurfaceSpecification modifications;
    modifications.parent = Window{};

    basic_window_manager.modify_surface(session, child, modifications);
}
