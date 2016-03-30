/*
 * Copyright © 2016 Canonical Ltd.
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

#include "mir_toolkit/mir_error.h"
#include "src/client/mir_error.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

TEST(MirError, error_has_domain)
{
    using namespace testing;

    MirError error{mir_error_domain_connection, 0};

    EXPECT_THAT(mir_error_get_domain(&error), Eq(mir_error_domain_connection));
}

TEST(MirError, error_has_code)
{
    using namespace testing;

    MirError error{mir_error_domain_connection, mir_connection_error_unauthorized_display_configuration};

    EXPECT_THAT(mir_error_get_code(&error), mir_connection_error_unauthorized_display_configuration);
}

