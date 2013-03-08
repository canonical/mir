/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#include "mir/shell/nullshell.h"
#include <gtest/gtest.h>

using namespace mir::shell;

TEST(NullShell, surface_attribs)
{
    NullShell n;

    EXPECT_TRUE(n.supports(mir_surface_attrib_type));
    EXPECT_FALSE(n.supports(static_cast<MirSurfaceAttrib>(-1)));
    EXPECT_FALSE(n.supports(static_cast<MirSurfaceAttrib>(99)));
    EXPECT_FALSE(n.supports(mir_surface_attrib_arraysize_));
}

TEST(NullShell, surface_types)
{
    NullShell n;

    EXPECT_TRUE(n.supports(mir_surface_attrib_type, mir_surface_type_normal));
    EXPECT_TRUE(n.supports(mir_surface_attrib_type, mir_surface_type_utility));
    EXPECT_TRUE(n.supports(mir_surface_attrib_type, mir_surface_type_dialog));
    EXPECT_TRUE(n.supports(mir_surface_attrib_type, mir_surface_type_overlay));
    EXPECT_TRUE(n.supports(mir_surface_attrib_type,
                           mir_surface_type_freestyle));
    EXPECT_TRUE(n.supports(mir_surface_attrib_type, mir_surface_type_popover));
    EXPECT_FALSE(n.supports(mir_surface_attrib_type,
                            mir_surface_type_arraysize_));
    EXPECT_FALSE(n.supports(mir_surface_attrib_type, -1));
    EXPECT_FALSE(n.supports(mir_surface_attrib_type, 123));

    EXPECT_FALSE(n.supports(static_cast<MirSurfaceAttrib>(-1),
                            mir_surface_type_normal));
    EXPECT_FALSE(n.supports(static_cast<MirSurfaceAttrib>(99),
                            mir_surface_type_normal));
    EXPECT_FALSE(n.supports(mir_surface_attrib_arraysize_,
                            mir_surface_type_normal));
}
