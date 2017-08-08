/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/frontend/session_credentials.h"

#include "mir_toolkit/mir_client_library.h"
#include "mir_toolkit/mir_screencast.h"

#include "mir_test_framework/connected_client_headless_server.h"
#include "mir/test/doubles/stub_session_authorizer.h"
#include "mir/test/fake_shared.h"
#include "mir/test/doubles/mock_gl.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mtf = mir_test_framework;
namespace mtd = mir::test::doubles;
namespace mf = mir::frontend;
namespace mt = mir::test;
using namespace testing;
using namespace std::literals::chrono_literals;

namespace
{
unsigned int const default_width{1};
unsigned int const default_height{1};
MirPixelFormat const default_pixel_format{mir_pixel_format_abgr_8888};
MirRectangle const default_capture_region{0, 0, 1, 1};

struct MockSessionAuthorizer : public mtd::StubSessionAuthorizer
{
    MOCK_METHOD1(screencast_is_allowed, bool(mf::SessionCredentials const&));
};

struct Screencast : mtf::HeadlessInProcessServer
{
    MockSessionAuthorizer mock_authorizer;
    NiceMock<mtd::MockGL> mockgl;

    void SetUp() override
    {
        server.override_the_session_authorizer([this]
            { return mt::fake_shared(mock_authorizer); });

        mtf::HeadlessInProcessServer::SetUp();
    }

    MirScreencastSpec* create_default_screencast_spec(MirConnection* connection)
    {
        MirScreencastSpec* spec = mir_create_screencast_spec(connection);
        mir_screencast_spec_set_width(spec, default_width);
        mir_screencast_spec_set_height(spec, default_height);
        mir_screencast_spec_set_pixel_format(spec, default_pixel_format);
        mir_screencast_spec_set_capture_region(spec, &default_capture_region);

        return spec;
    }
};
}

// TODO test case(s) showing screencast works. lp:1396681

TEST_F(Screencast, with_invalid_params_fails)
{
    using namespace testing;

    EXPECT_CALL(mock_authorizer, screencast_is_allowed(_))
        .WillOnce(Return(true));

    auto const connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    auto spec = create_default_screencast_spec(connection);

    mir_screencast_spec_set_height(spec, 0);
    auto screencast = mir_screencast_create_sync(spec);
    EXPECT_FALSE(mir_screencast_is_valid(screencast));

    mir_screencast_release_sync(screencast);

    mir_screencast_spec_set_height(spec, default_height);

    MirRectangle invalid_region{0, 0, 1, 0};
    mir_screencast_spec_set_capture_region(spec, &invalid_region);
    screencast = mir_screencast_create_sync(spec);
    EXPECT_FALSE(mir_screencast_is_valid(screencast));

    mir_screencast_release_sync(screencast);

    mir_screencast_spec_set_capture_region(spec, &default_capture_region);
    mir_screencast_spec_set_pixel_format(spec, mir_pixel_format_invalid);

    screencast = mir_screencast_create_sync(spec);
    EXPECT_FALSE(mir_screencast_is_valid(screencast));

    mir_screencast_spec_release(spec);
    mir_screencast_release_sync(screencast);
    mir_connection_release(connection);
}

TEST_F(Screencast, when_unauthorized_fails)
{
    using namespace testing;

    EXPECT_CALL(mock_authorizer, screencast_is_allowed(_))
        .WillOnce(Return(false));

    auto const connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    auto spec = create_default_screencast_spec(connection);
    auto screencast = mir_screencast_create_sync(spec);
    EXPECT_FALSE(mir_screencast_is_valid(screencast));

    mir_screencast_spec_release(spec);
    mir_screencast_release_sync(screencast);
    mir_connection_release(connection);
}

TEST_F(Screencast, can_cast_to_buffer)
{
    EXPECT_CALL(mock_authorizer, screencast_is_allowed(_))
        .WillOnce(Return(true));
    auto const connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    auto const buffer = mir_connection_allocate_buffer_sync(
        connection, default_width, default_height, default_pixel_format);

    ASSERT_TRUE(buffer);

    MirScreencastSpec* spec = mir_create_screencast_spec(connection);
    //We have to set nbuffers == 0 now to avoid capturing at startup. Current default is 1.
    mir_screencast_spec_set_number_of_buffers(spec, 0);

    mir_screencast_spec_set_capture_region(spec, &default_capture_region);
    auto screencast = mir_screencast_create_sync(spec);
    mir_screencast_spec_release(spec);

    struct Capture
    {
        std::mutex mutex;
        std::condition_variable cv;
        bool capture = false;
    } capture;

    mir_screencast_capture_to_buffer(screencast, buffer,
        [] (MirScreencastResult /*status*/, MirBuffer* /*buffer*/, void* context) {
            auto c = reinterpret_cast<Capture*>(context);
            std::unique_lock<decltype(c->mutex)> lk(c->mutex);
            c->capture = true;
            c->cv.notify_all();
        }, &capture );

    std::unique_lock<decltype(capture.mutex)> lk2(capture.mutex);
    ASSERT_TRUE(capture.cv.wait_for(lk2, 5s, [&] { return capture.capture; }));
    EXPECT_THAT(mir_buffer_get_error_message(buffer), StrEq(""));

    mir_screencast_release_sync(screencast);
    mir_connection_release(connection);
}

TEST_F(Screencast, can_cast_to_buffer_sync)
{
    EXPECT_CALL(mock_authorizer, screencast_is_allowed(_))
        .WillOnce(Return(true));
    auto const connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    struct BufferSync
    {
        MirBuffer* buffer = nullptr;
        std::mutex mutex;
        std::condition_variable cv;
    } buffer_info;

    mir_connection_allocate_buffer(
        connection,
        default_width, default_height, default_pixel_format,
        [](MirBuffer* b, void* ctxt) {
            auto info = reinterpret_cast<BufferSync*>(ctxt);
            std::unique_lock<decltype(info->mutex)> lk(info->mutex);
            info->buffer = b;
            info->cv.notify_all(); 
        }, &buffer_info);
    std::unique_lock<decltype(buffer_info.mutex)> lk(buffer_info.mutex);
    ASSERT_TRUE(buffer_info.cv.wait_for(lk, 5s, [&] { return buffer_info.buffer; }));

    MirScreencastSpec* spec = mir_create_screencast_spec(connection);
    //We have to set nbuffers == 0 now to avoid capturing at startup. Current default is 1.
    mir_screencast_spec_set_number_of_buffers(spec, 0);

    mir_screencast_spec_set_capture_region(spec, &default_capture_region);
    auto screencast = mir_screencast_create_sync(spec);

    mir_screencast_spec_release(spec);
    mir_screencast_capture_to_buffer_sync(screencast, buffer_info.buffer);

    mir_screencast_release_sync(screencast);
    mir_connection_release(connection);
}
