/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include <mir_toolkit/mir_client_library.h>

#include <gtest/gtest.h>

TEST(ClientHeaderVersion, mir_client_version_ge_is_sane)
{
    // We have to be greater than 0.0.0
    EXPECT_TRUE(MIR_CLIENT_VERSION_GE(0, 0, 0));
    // Our client API has been stable for a while; we're past 1.0.0
    EXPECT_TRUE(MIR_CLIENT_VERSION_GE(1, 0, 0));
    // We should be greater than or equal to our current version
    EXPECT_TRUE(MIR_CLIENT_VERSION_GE(MIR_CLIENT_MAJOR_VERSION,
				      MIR_CLIENT_MINOR_VERSION,
				      MIR_CLIENT_MICRO_VERSION));
    
    EXPECT_FALSE(MIR_CLIENT_VERSION_GE(MIR_CLIENT_MAJOR_VERSION + 1, 0, 0));

    EXPECT_FALSE(MIR_CLIENT_VERSION_GE(MIR_CLIENT_MAJOR_VERSION,
				       MIR_CLIENT_MINOR_VERSION + 1,
				       0));
    
    EXPECT_FALSE(MIR_CLIENT_VERSION_GE(MIR_CLIENT_MAJOR_VERSION,
				       MIR_CLIENT_MINOR_VERSION,
				       MIR_CLIENT_MICRO_VERSION + 1));
}

TEST(ClientHeaderVersion, mir_client_version_ge_is_usable_in_preprocessor)
{
#if !MIR_CLIENT_VERSION_GE(0, 0, 0)
    FAIL() << "!MIR_CLIENT_VERSION_GE(0, 0, 0)";
#endif
    
#if !MIR_CLIENT_VERSION_GE(MIR_CLIENT_MAJOR_VERSION, MIR_CLIENT_MINOR_VERSION, MIR_CLIENT_MICRO_VERSION)
    FAIL() << "!MIR_CLIENT_VERSION_GE(MIR_CLIENT_MAJOR_VERSION, MIR_CLIENT_MINOR_VERSION, MIR_CLIENT_MICRO_VERSION)";
#endif
    
#if MIR_CLIENT_VERSION_GE(MIR_CLIENT_MAJOR_VERSION + 1, 0, 0)
    FAIL() << "MIR_CLIENT_VERSION_GE(MIR_CLIENT_MAJOR_VERSION + 1, 0, 0)";

#endif

#if MIR_CLIENT_VERSION_GE(MIR_CLIENT_MAJOR_VERSION, MIR_CLIENT_MINOR_VERSION + 1, 0)
    FAIL() << "MIR_CLIENT_VERSION_GE(MIR_CLIENT_MAJOR_VERSION, MIR_CLIENT_MINOR_VERSION + 1, 0)";

#endif
    
#if MIR_CLIENT_VERSION_GE(MIR_CLIENT_MAJOR_VERSION, MIR_CLIENT_MINOR_VERSION, MIR_CLIENT_MICRO_VERSION + 1)
    FAIL() << "MIR_CLIENT_VERSION_GE(MIR_CLIENT_MAJOR_VERSION, MIR_CLIENT_MINOR_VERSION, MIR_CLIENT_MICRO_VERSION + 1)";
#endif
}
