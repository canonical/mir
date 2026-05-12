/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "src/server/frontend_wayland/foreign_toplevel_handle_creation.h"
#include <mir/test/doubles/mock_scene_session.h>
#include <mir/test/doubles/mock_surface.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace mtd = mir::test::doubles;
using namespace testing;

namespace
{

class ForeignToplevelHandleCreation : public Test
{
public:
    NiceMock<mtd::MockSurface> surface;
    std::shared_ptr<NiceMock<mtd::MockSceneSession>> session{
        std::make_shared<NiceMock<mtd::MockSceneSession>>()};

    void SetUp() override
    {
        ON_CALL(surface, type())
            .WillByDefault(Return(mir_window_type_normal));
        ON_CALL(surface, session())
            .WillByDefault(Return(std::weak_ptr<ms::Session>{session}));
    }
};

}

TEST_F(ForeignToplevelHandleCreation, creates_handles_for_application_layer_normal_windows)
{
    EXPECT_TRUE(mf::should_create_foreign_toplevel_handle(surface, "app.id"));
}

TEST_F(ForeignToplevelHandleCreation, creates_handles_for_freestyle_windows)
{
    ON_CALL(surface, type())
        .WillByDefault(Return(mir_window_type_freestyle));
    EXPECT_TRUE(mf::should_create_foreign_toplevel_handle(surface, "app.id"));
}

TEST_F(ForeignToplevelHandleCreation, does_not_create_handles_for_non_application_layer_windows)
{
    surface.set_depth_layer(mir_depth_layer_above);
    EXPECT_FALSE(mf::should_create_foreign_toplevel_handle(surface, "app.id"));
}

TEST_F(ForeignToplevelHandleCreation, does_not_create_handles_for_non_focusable_windows)
{
    surface.set_focus_mode(mir_focus_mode_disabled);
    EXPECT_FALSE(mf::should_create_foreign_toplevel_handle(surface, "app.id"));
}

TEST_F(ForeignToplevelHandleCreation, does_not_create_handles_without_session)
{
    ON_CALL(surface, session())
        .WillByDefault(Return(std::weak_ptr<ms::Session>{}));
    EXPECT_FALSE(mf::should_create_foreign_toplevel_handle(surface, "app.id"));
}

TEST_F(ForeignToplevelHandleCreation, does_not_create_handles_with_empty_app_id)
{
    EXPECT_FALSE(mf::should_create_foreign_toplevel_handle(surface, ""));
}
