/*
 * Copyright © 2015 Canonical Ltd.
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
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "mir/cookie/authority.h"
#include "mir/cookie/cookie.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <chrono>
#include <fcntl.h>

namespace {

void drain_dev_random()
{
    // Flush the entropy pool
    int fd = open("/dev/random", O_RDONLY | O_NONBLOCK);
    ASSERT_THAT(fd, ::testing::Ge(0));
    char buf[256];
    while (read(fd, buf, sizeof buf) > 0) {}
    close(fd);
}

} // anonymous namespace


TEST(MirCookieAuthority, attests_real_timestamp)
{
    std::vector<uint8_t> secret{ 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0xde, 0x01 };
    auto authority = mir::cookie::Authority::create_from(secret);

    uint64_t mock_timestamp{0x322322322332};

    auto cookie = authority->make_cookie(mock_timestamp);
    EXPECT_NO_THROW({
        authority->make_cookie(cookie->serialize());
    });
}

TEST(MirCookieAuthority, doesnt_attest_faked_mac)
{
    std::vector<uint8_t> secret{ 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0xde, 0x01 };
    auto authority = mir::cookie::Authority::create_from(secret);

    std::vector<uint8_t> cookie{ 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0xde, 0x01 };

    EXPECT_THROW({
        authority->make_cookie(cookie);
    }, mir::cookie::SecurityCheckError);
}

TEST(MirCookieAuthority, timestamp_trusted_with_different_secret_doesnt_attest)
{
    std::vector<uint8_t> alice{ 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0xde, 0x01 };
    std::vector<uint8_t> bob{ 0x01, 0x02, 0x44, 0xd8, 0xee, 0x0f, 0xde, 0x01 };

    auto alices_authority = mir::cookie::Authority::create_from(alice);
    auto bobs_authority   = mir::cookie::Authority::create_from(bob);

    uint64_t mock_timestamp{0x01020304};

    EXPECT_THROW({
        auto alices_cookie = alices_authority->make_cookie(mock_timestamp);
        auto bobs_cookie   = bobs_authority->make_cookie(mock_timestamp);

        alices_authority->make_cookie(bobs_cookie->serialize());
        bobs_authority->make_cookie(alices_cookie->serialize());
    }, mir::cookie::SecurityCheckError);
}

TEST(MirCookieAuthority, throw_when_secret_size_to_small)
{
    std::vector<uint8_t> bob(mir::cookie::Authority::minimum_secret_size - 1);
    EXPECT_THROW({
        auto authority = mir::cookie::Authority::create_from(bob);
    }, std::logic_error);
}

TEST(MirCookieAuthority, saves_a_secret)
{
    using namespace testing;
    std::vector<uint8_t> secret;

    mir::cookie::Authority::create_saving(secret);

    EXPECT_THAT(secret.size(), Ge(mir::cookie::Authority::minimum_secret_size));
}

TEST(MirCookieAuthority, timestamp_trusted_with_saved_secret_does_attest)
{
    uint64_t timestamp   = 23;
    std::vector<uint8_t> secret;

    auto source_authority = mir::cookie::Authority::create_saving(secret);
    auto sink_authority   = mir::cookie::Authority::create_from(secret);
    auto cookie = source_authority->make_cookie(timestamp);

    EXPECT_NO_THROW({
        sink_authority->make_cookie(cookie->serialize());
    });
}

TEST(MirCookieAuthority, internally_generated_secret_has_optimum_size)
{
    using namespace testing;
    std::vector<uint8_t> secret;

    mir::cookie::Authority::create_saving(secret);

    EXPECT_THAT(secret.size(), Eq(mir::cookie::Authority::optimal_secret_size()));
}

TEST(MirCookieAuthority, optimal_secret_size_is_larger_than_minimum_size)
{
    using namespace testing;

    EXPECT_THAT(mir::cookie::Authority::optimal_secret_size(),
        Ge(mir::cookie::Authority::minimum_secret_size));
}

TEST(MirCookieAuthority, DISABLED_given_low_entropy_does_not_hang_or_crash)
{   // Regression test for LP: #1536662 and LP: #1541188
    using namespace testing;

    drain_dev_random();

    auto start = std::chrono::high_resolution_clock::now();

    EXPECT_NO_THROW( mir::cookie::Authority::create() );

    auto duration = std::chrono::high_resolution_clock::now() - start;
    int seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
    EXPECT_THAT(seconds, Lt(15));
}

TEST(MirCookieAuthority, DISABLED_makes_cookies_quickly)
{   // Regression test for LP: #1536662 and LP: #1541188
    using namespace testing;

    drain_dev_random();
    uint64_t timestamp = 23;
    std::vector<uint8_t> secret;
    auto source_authority = mir::cookie::Authority::create_saving(secret);

    drain_dev_random();
    auto start = std::chrono::high_resolution_clock::now();
    auto cookie = source_authority->make_cookie(timestamp);
    auto duration = std::chrono::high_resolution_clock::now() - start;
    int seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
    EXPECT_THAT(seconds, Lt(5));
}
