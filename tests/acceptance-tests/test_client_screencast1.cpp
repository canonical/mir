/*
 * Copyright © 2017 Canonical Ltd.
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
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "client_screencast.h"
#include "mir/frontend/session_credentials.h"
#include "mir_toolkit/mir_buffer.h"
#include "mir_toolkit/mir_client_library.h"
#include "mir_toolkit/mir_screencast.h"

namespace mt = mir::test;
using namespace testing;
using namespace std::literals::chrono_literals;

typedef mt::ScreencastBase ScreencastToBuffer;
TEST_F(ScreencastToBuffer, can_cast_to_buffer)
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
        default_width, default_height, default_pixel_format, mir_buffer_usage_software,
        [](MirBuffer* b, void* ctxt) {
            auto info = reinterpret_cast<BufferSync*>(ctxt);
            std::unique_lock<decltype(info->mutex)> lk(info->mutex);
            info->buffer = b;
            info->cv.notify_all(); 
        }, &buffer_info);
    std::unique_lock<decltype(buffer_info.mutex)> lk(buffer_info.mutex);
    ASSERT_TRUE(buffer_info.cv.wait_for(lk, 5s, [&] { return buffer_info.buffer; }));

    MirScreencastSpec* spec = mir_create_screencast_spec(connection);

    //TODO: makes no sense:

    mir_screencast_spec_set_number_of_buffers(spec, 0);
    mir_screencast_spec_set_width(spec, default_width);
    mir_screencast_spec_set_height(spec, default_height);
    mir_screencast_spec_set_pixel_format(spec, default_pixel_format);

    mir_screencast_spec_set_capture_region(spec, &default_capture_region);
    auto screencast = mir_screencast_create_sync(spec);

    struct Capture
    {
        std::mutex mutex;
        std::condition_variable cv;
        bool capture = false;
    } capture;

    mir_screencast_capture_to_buffer(screencast, buffer_info.buffer, 
        [] (MirBuffer*, void* context) {
            auto c = reinterpret_cast<Capture*>(context);
            std::unique_lock<decltype(c->mutex)> lk(c->mutex);
            c->capture = true;
            c->cv.notify_all();
        }, &capture );

    std::unique_lock<decltype(capture.mutex)> lk2(capture.mutex);
    ASSERT_TRUE(capture.cv.wait_for(lk2, 5s, [&] { return capture.capture; }));
    EXPECT_THAT(mir_buffer_get_error_message(buffer_info.buffer), StrEq(""));

    mir_screencast_release_sync(screencast);
    mir_connection_release(connection);
}
