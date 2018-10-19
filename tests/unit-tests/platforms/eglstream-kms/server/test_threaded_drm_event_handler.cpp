/*
 * Copyright © 2018 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "src/platforms/eglstream-kms/server/threaded_drm_event_handler.h"

#include "mir/test/doubles/mock_drm.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mge = mir::graphics::eglstream;
namespace mtd = mir::test::doubles;

using namespace testing;

class ThreadedDRMEventHandlerTest : public testing::Test
{
public:
    ThreadedDRMEventHandlerTest()
        : mock_drm_fd{mock_drm.open(device_node, 0, 0)}
    {
    }

    void add_flip_event(
        unsigned int frame_no,
        unsigned int tv_sec,
        unsigned int tv_usec,
        uint32_t crtc_id,
        void const* userdata)
    {
        EXPECT_CALL(mock_drm, drmHandleEvent(static_cast<int>(mock_drm_fd), _))
            .Times(AtMost(1))
            .WillRepeatedly(
                Invoke(
                    [this, frame_no, tv_sec, tv_usec, crtc_id, userdata](
                        auto drm_fd,
                        drmEventContextPtr event_context) -> int
                    {
                        EXPECT_THAT(event_context->version, Ge(3));
                        EXPECT_THAT(event_context->page_flip_handler2, NotNull());
                        if (!event_context->page_flip_handler2)
                            return -1;

                        event_context->page_flip_handler2(
                            drm_fd,
                            frame_no,
                            tv_sec,
                            tv_usec,
                            crtc_id,
                            const_cast<void*>(userdata));

                        mock_drm.consume_event_on(device_node);
                        return 0;
                    }));
        mock_drm.generate_event_on(device_node);
    }

    ::testing::NiceMock<mtd::MockDRM> mock_drm;

    char const* const device_node = "/dev/dri/card0";
    mir::Fd mock_drm_fd;
};

TEST_F(ThreadedDRMEventHandlerTest, flip_completion_handle_becomes_ready_on_flip)
{
    using namespace std::literals::chrono_literals;

    mge::ThreadedDRMEventHandler handler{mock_drm_fd};
    mge::ThreadedDRMEventHandler::KMSCrtcId const crtc_id{55};

    auto completion_handle = handler.expect_flip_event(crtc_id, [](auto, auto){});

    ASSERT_TRUE(completion_handle.valid());
    EXPECT_THAT(completion_handle.wait_for(0ms), Not(Eq(std::future_status::ready)));

    add_flip_event(0, 0, 0, crtc_id, handler.drm_event_data());

    EXPECT_THAT(completion_handle.wait_for(30s), Eq(std::future_status::ready));
}

TEST_F(ThreadedDRMEventHandlerTest, calls_frame_callback_on_flip_completion)
{
    using namespace std::literals::chrono_literals;

    mge::ThreadedDRMEventHandler handler{mock_drm_fd};
    mge::ThreadedDRMEventHandler::KMSCrtcId const crtc_id{55};
    int constexpr frame_sec{10};
    int constexpr frame_usec{400};
    unsigned int constexpr expected_frame_no{3441};

    auto completion_handle = handler.expect_flip_event(
        crtc_id,
        [frame_sec, frame_usec](unsigned int frame_no, std::chrono::milliseconds frame_time)
        {
            EXPECT_THAT(frame_no, Eq(expected_frame_no));
            EXPECT_THAT(frame_time, Eq(std::chrono::seconds{frame_sec} + std::chrono::milliseconds{frame_usec}));
        });

    add_flip_event(expected_frame_no, frame_sec, frame_usec, crtc_id, handler.drm_event_data());
    EXPECT_THAT(completion_handle.wait_for(30s), Eq(std::future_status::ready));
}

TEST_F(ThreadedDRMEventHandlerTest, handles_spurious_drm_events)
{
    using namespace std::literals::chrono_literals;

    mge::ThreadedDRMEventHandler handler{mock_drm_fd};
    mge::ThreadedDRMEventHandler::KMSCrtcId const crtc_id{55};
    int constexpr frame_sec{10};
    int constexpr frame_usec{400};
    unsigned int constexpr expected_frame_no{3441};

    auto completion_handle = handler.expect_flip_event(
        crtc_id,
        [frame_sec, frame_usec](unsigned int frame_no, std::chrono::milliseconds frame_time)
        {
            EXPECT_THAT(frame_no, Eq(expected_frame_no));
            EXPECT_THAT(frame_time, Eq(std::chrono::seconds{frame_sec} + std::chrono::milliseconds{frame_usec}));
        });

    add_flip_event(23, 10, 10, 5, handler.drm_event_data());
    add_flip_event(33, 20, 0, 10, handler.drm_event_data());
    add_flip_event(expected_frame_no, frame_sec, frame_usec, crtc_id, handler.drm_event_data());

    EXPECT_THAT(completion_handle.wait_for(30s), Eq(std::future_status::ready));
}

TEST_F(ThreadedDRMEventHandlerTest, dispatches_event_for_correct_crtc)
{
    using namespace std::literals::chrono_literals;

    mge::ThreadedDRMEventHandler handler{mock_drm_fd};
    mge::ThreadedDRMEventHandler::KMSCrtcId const crtc_one{55};
    mge::ThreadedDRMEventHandler::KMSCrtcId const crtc_two{44};

    std::atomic<bool> first_flip_done{false};
    std::atomic<bool> second_flip_done{false};

    auto first_handle = handler.expect_flip_event(
        crtc_one,
        [&first_flip_done](auto, auto){ first_flip_done = true; });
    auto second_handle = handler.expect_flip_event(
        crtc_two,
        [&second_flip_done](auto, auto){ second_flip_done = true; });

    add_flip_event(0, 0, 0, crtc_two, handler.drm_event_data());

    EXPECT_THAT(second_handle.wait_for(30s), Eq(std::future_status::ready));
    EXPECT_TRUE(second_flip_done);
    EXPECT_FALSE(first_flip_done);

    add_flip_event(0, 0, 0, crtc_one, handler.drm_event_data());
    EXPECT_THAT(first_handle.wait_for(30s), Eq(std::future_status::ready));
    EXPECT_TRUE(first_flip_done);
}
