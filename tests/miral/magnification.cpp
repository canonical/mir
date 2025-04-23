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

#include <miral/test_server.h>
#include <miral/magnification.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace testing;

struct Magnification : miral::TestServer
{
    Magnification() : magnification(false)
    {
        start_server_in_setup = false;
    }

    void SetUp() override
    {
        add_server_init(magnification);
        TestServer::SetUp();
    }

    miral::Magnification magnification;
};

TEST_F(Magnification, can_be_enabled)
{
    start_server();

    class MockCallback {
    public:
        MOCK_METHOD(void, callback_function, (), ());
    };

    MockCallback mock_callback;
    EXPECT_CALL(mock_callback, callback_function)
        .Times(1);

    magnification.on_enabled([&] { mock_callback.callback_function(); });
    EXPECT_THAT(magnification.enabled(true), Eq(true));
}

TEST_F(Magnification, can_be_enabled_before_server_started)
{
    class MockCallback {
    public:
        MOCK_METHOD(void, callback_function, (), ());
    };

    MockCallback mock_callback;
    EXPECT_CALL(mock_callback, callback_function)
        .Times(1);

    magnification.on_enabled([&] { mock_callback.callback_function(); });
    EXPECT_THAT(magnification.enabled(true), Eq(true));

    start_server();
}

TEST_F(Magnification, can_be_disabled)
{
    start_server();

    class MockCallback {
    public:
        MOCK_METHOD(void, callback_function, (), ());
    };

    MockCallback mock_callback;
    EXPECT_CALL(mock_callback, callback_function)
        .Times(1);

    magnification.on_disabled([&] { mock_callback.callback_function(); });
    EXPECT_THAT(magnification.enabled(true), Eq(true));
    EXPECT_THAT(magnification.enabled(false), Eq(true));
}

TEST_F(Magnification, can_be_magnified)
{
    start_server();

    magnification.magnification(5.f);
    EXPECT_THAT(magnification.magnification(), Eq(5.f));
}

TEST_F(Magnification, can_be_magnified_before_server_started)
{
    magnification.magnification(5.f);
    start_server();
    EXPECT_THAT(magnification.magnification(), Eq(5.f));
}

TEST_F(Magnification, can_be_resized)
{
    start_server();

    magnification.size(mir::geometry::Size{100, 100});
    EXPECT_THAT(magnification.size(), Eq(mir::geometry::Size{100, 100}));
}

TEST_F(Magnification, can_be_resized_before_server_started)
{
    magnification.size(mir::geometry::Size{100, 100});
    start_server();
    EXPECT_THAT(magnification.size(), Eq(mir::geometry::Size{100, 100}));
}
