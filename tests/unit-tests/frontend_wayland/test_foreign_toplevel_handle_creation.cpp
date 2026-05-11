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

#include <gtest/gtest.h>

namespace mf = mir::frontend;

TEST(ForeignToplevelHandleCreation, prefers_application_id_from_app)
{
    EXPECT_EQ(mf::foreign_toplevel_app_id("app.id", "resolved.desktop"), "app.id");
}

TEST(ForeignToplevelHandleCreation, falls_back_to_resolved_app_id_when_app_id_is_empty)
{
    EXPECT_EQ(mf::foreign_toplevel_app_id("", "resolved.desktop"), "resolved.desktop");
}

TEST(ForeignToplevelHandleCreation, creates_handles_for_normal_windows_with_session_and_app_id)
{
    EXPECT_TRUE(mf::should_create_foreign_toplevel_handle(mir_window_type_normal, true, "app.id"));
}

TEST(ForeignToplevelHandleCreation, does_not_create_handles_for_non_normal_window_types)
{
    EXPECT_FALSE(mf::should_create_foreign_toplevel_handle(mir_window_type_utility, true, "app.id"));
}

TEST(ForeignToplevelHandleCreation, does_not_create_handles_without_session)
{
    EXPECT_FALSE(mf::should_create_foreign_toplevel_handle(mir_window_type_normal, false, "app.id"));
}

TEST(ForeignToplevelHandleCreation, does_not_create_handles_with_empty_app_id)
{
    EXPECT_FALSE(mf::should_create_foreign_toplevel_handle(mir_window_type_normal, true, ""));
}
