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

    EXPECT_TRUE(n.supports(MIR_SURFACE_TYPE));
    EXPECT_FALSE(n.supports(static_cast<MirSurfaceAttrib>(-1)));
    EXPECT_FALSE(n.supports(static_cast<MirSurfaceAttrib>(99)));
    EXPECT_FALSE(n.supports(MirSurfaceAttrib_ARRAYSIZE));
}

TEST(NullShell, surface_types)
{
    NullShell n;

    EXPECT_TRUE(n.supports(MIR_SURFACE_TYPE, MIR_SURFACE_NORMAL));
    EXPECT_TRUE(n.supports(MIR_SURFACE_TYPE, MIR_SURFACE_UTILITY));
    EXPECT_TRUE(n.supports(MIR_SURFACE_TYPE, MIR_SURFACE_DIALOG));
    EXPECT_TRUE(n.supports(MIR_SURFACE_TYPE, MIR_SURFACE_OVERLAY));
    EXPECT_TRUE(n.supports(MIR_SURFACE_TYPE, MIR_SURFACE_FREESTYLE));
    EXPECT_TRUE(n.supports(MIR_SURFACE_TYPE, MIR_SURFACE_POPOVER));
    EXPECT_FALSE(n.supports(MIR_SURFACE_TYPE, MirSurfaceType_ARRAYSIZE));
    EXPECT_FALSE(n.supports(MIR_SURFACE_TYPE, -1));
    EXPECT_FALSE(n.supports(MIR_SURFACE_TYPE, 123));

    EXPECT_FALSE(n.supports(static_cast<MirSurfaceAttrib>(-1),
                            MIR_SURFACE_NORMAL));
    EXPECT_FALSE(n.supports(static_cast<MirSurfaceAttrib>(99),
                            MIR_SURFACE_NORMAL));
    EXPECT_FALSE(n.supports(MirSurfaceAttrib_ARRAYSIZE, MIR_SURFACE_NORMAL));
}
