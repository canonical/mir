/*
 * Copyright © 2015 Canonical Ltd.
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

#include "mir_toolkit/mir_client_library.h"
#include "mir/cookie_factory.h"

#include "mir_test_framework/headless_test.h"
#include "mir_test_framework/connected_client_with_a_surface.h"
#include "mir/test/doubles/wrap_shell_to_track_latest_surface.h"
#include "mir/shell/shell_wrapper.h"
#include "mir/test/validity_matchers.h"
#include "mir/test/wait_condition.h"

#include "boost/throw_exception.hpp"

#include <gtest/gtest.h>

namespace mtf = mir_test_framework;
namespace mtd = mir::test::doubles;
namespace msh = mir::shell;

TEST(MirCookieFactory, attests_real_timestamp)
{
    std::vector<uint8_t> secret{ 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0xde, 0x01 };
    mir::CookieFactory factory{secret};

    uint64_t mock_timestamp{0x322322322332};

    auto cookie = factory.timestamp_to_cookie(mock_timestamp);

    EXPECT_TRUE(factory.attest_timestamp(cookie));
}

TEST(MirCookieFactory, doesnt_attest_faked_timestamp)
{
    std::vector<uint8_t> secret{ 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0xde, 0x01 };
    mir::CookieFactory factory{secret};

    MirCookie bad_client_no_biscuit{ 0x33221100, 0x33221100 };

    EXPECT_FALSE(factory.attest_timestamp(bad_client_no_biscuit));
}

TEST(MirCookieFactory, timestamp_trusted_with_different_secret_doesnt_attest)
{
    std::vector<uint8_t> alice{ 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0xde, 0x01 };
    std::vector<uint8_t> bob{ 0x01, 0x02, 0x44, 0xd8, 0xee, 0x0f, 0xde, 0x01 };

    mir::CookieFactory alices_factory{alice};
    mir::CookieFactory bobs_factory{bob};

    uint64_t mock_timestamp{0x01020304};

    auto alices_cookie = alices_factory.timestamp_to_cookie(mock_timestamp);
    auto bobs_cookie = bobs_factory.timestamp_to_cookie(mock_timestamp);

    EXPECT_FALSE(alices_factory.attest_timestamp(bobs_cookie));
    EXPECT_FALSE(bobs_factory.attest_timestamp(alices_cookie));
}

TEST(MirCookieFactory, throw_when_secret_size_to_small)
{
    std::vector<uint8_t> bob{ 0x01 };
    EXPECT_THROW({
        mir::CookieFactory factory{bob};
    }, std::logic_error);
}
