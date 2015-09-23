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

#include "mir/cookie_factory.h"

#include <gtest/gtest.h>

TEST(MirCookieFactory, attests_real_timestamp)
{
    std::vector<uint8_t> secret{ 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0xde, 0x01 };
    auto factory = mir::cookie::CookieFactory::create_from_secret(secret);

    uint64_t mock_timestamp{0x322322322332};

    auto cookie = factory->timestamp_to_cookie(mock_timestamp);

    EXPECT_TRUE(factory->attest_timestamp(cookie));
}

TEST(MirCookieFactory, doesnt_attest_faked_timestamp)
{
    std::vector<uint8_t> secret{ 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0xde, 0x01 };
    auto factory = mir::cookie::CookieFactory::create_from_secret(secret);

    MirCookie bad_client_no_biscuit{ 0x33221100, 0x33221100 };

    EXPECT_FALSE(factory->attest_timestamp(bad_client_no_biscuit));
}

TEST(MirCookieFactory, timestamp_trusted_with_different_secret_doesnt_attest)
{
    std::vector<uint8_t> alice{ 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0xde, 0x01 };
    std::vector<uint8_t> bob{ 0x01, 0x02, 0x44, 0xd8, 0xee, 0x0f, 0xde, 0x01 };

    auto alices_factory = mir::cookie::CookieFactory::create_from_secret(alice);
    auto bobs_factory   = mir::cookie::CookieFactory::create_from_secret(bob);

    uint64_t mock_timestamp{0x01020304};

    auto alices_cookie = alices_factory->timestamp_to_cookie(mock_timestamp);
    auto bobs_cookie = bobs_factory->timestamp_to_cookie(mock_timestamp);

    EXPECT_FALSE(alices_factory->attest_timestamp(bobs_cookie));
    EXPECT_FALSE(bobs_factory->attest_timestamp(alices_cookie));
}

TEST(MirCookieFactory, throw_when_secret_size_to_small)
{
    std::vector<uint8_t> bob(mir::cookie::CookieFactory::minimum_secret_size - 1);
    EXPECT_THROW({
        auto factory = mir::cookie::CookieFactory::create_from_secret(bob);
    }, std::logic_error);
}

TEST(MirCookieFactory, saves_a_secret)
{
    std::vector<uint8_t> secret;
    unsigned secret_size = 64;

    mir::cookie::CookieFactory::create_saving_secret(secret, secret_size);

    EXPECT_EQ(secret.size(), secret_size);
}

TEST(MirCookieFactory, timestamp_trusted_with_saved_secret_does_attest)
{
    uint64_t timestamp   = 23;
    unsigned secret_size = 64;
    std::vector<uint8_t> secret;

    auto source_factory = mir::cookie::CookieFactory::create_saving_secret(secret, secret_size);
    auto sink_factory   = mir::cookie::CookieFactory::create_from_secret(secret);
    auto cookie = source_factory->timestamp_to_cookie(timestamp);

    EXPECT_TRUE(sink_factory->attest_timestamp(cookie));
}
