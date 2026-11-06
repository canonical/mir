/*
 * Copyright © Canonical Ltd.
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
 */

#include "src/platforms/atomic-kms/server/kms/display_sink.h"
#include "src/platforms/atomic-kms/server/kms/quirks.h"
#include "src/server/report/null_report_factory.h"
#include "mock_kms_framebuffer.h"
#include "mock_kms_output.h"

#include <mir/test/doubles/mock_drm.h>
#include <mir_test_framework/udev_environment.h>

#include <fcntl.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <optional>
#include <string_view>

namespace mg = mir::graphics;
namespace mga = mir::graphics::atomic;
namespace geom = mir::geometry;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

using namespace testing;

namespace
{
class StubSurfaceHasFreeBuffersQuirk : public mga::GbmQuirks::SurfaceHasFreeBuffersQuirk
{
public:
    auto gbm_surface_has_free_buffers(gbm_surface*) const -> int override
    {
        return 1;
    }
};

class AtomicDisplaySinkTest : public Test
{
protected:
    void SetUp() override
    {
        devices.add_standard_device("standard-drm-devices");
        drm_fd = mir::Fd{::open("/dev/dri/card0", O_RDWR | O_CLOEXEC)};
        ASSERT_GE(drm_fd, 0);

        ON_CALL(*output, has_crtc_mismatch())
            .WillByDefault(Return(false));
        ON_CALL(*output, max_refresh_rate())
            .WillByDefault(Return(60));

        sink = std::make_unique<mga::DisplaySink>(
            drm_fd,
            nullptr,
            mga::BypassOption::allowed,
            mga::PlanePromotion::available,
            mir::report::null_display_report(),
            output,
            display_area,
            glm::mat2{1},
            std::make_shared<mga::GbmQuirks>(std::make_unique<StubSurfaceHasFreeBuffersQuirk>()));
    }

    auto fullscreen_element(std::shared_ptr<mg::Framebuffer> const& framebuffer) const -> mg::DisplayElement
    {
        return {
            display_area,
            {{0, 0}, {display_area.size.width.as_int(), display_area.size.height.as_int()}},
            framebuffer};
    }

    geom::Rectangle const display_area{{12, 34}, {56, 78}};
    mir_test_framework::UdevEnvironment devices;
    NiceMock<mtd::MockDRM> drm;
    mir::Fd drm_fd;
    std::shared_ptr<NiceMock<mt::MockKMSOutput>> const output{
        std::make_shared<NiceMock<mt::MockKMSOutput>>()};
    std::unique_ptr<mga::DisplaySink> sink;
};

struct PostCase
{
    bool flip_ok;
    std::optional<bool> crtc_ok;
    bool expected;
};

class AtomicPostResult : public AtomicDisplaySinkTest, public WithParamInterface<PostCase>
{
};
}

TEST_P(AtomicPostResult, post_reports_presentation_result)
{
    auto const& param = GetParam();
    auto const framebuffer = std::make_shared<NiceMock<mt::MockKMSFramebuffer>>();
    ASSERT_TRUE(sink->overlay({fullscreen_element(framebuffer)}));

    InSequence sequence;
    EXPECT_CALL(*output, page_flip(Ref(*framebuffer)))
        .WillOnce(Return(param.flip_ok));
    if (param.crtc_ok.has_value())
    {
        EXPECT_CALL(*output, set_crtc(Ref(*framebuffer)))
            .WillOnce(Return(*param.crtc_ok));
    }
    else
    {
        EXPECT_CALL(*output, set_crtc(_))
            .Times(0);
    }

    EXPECT_EQ(param.expected, sink->post());
}

INSTANTIATE_TEST_SUITE_P(
    Presentation,
    AtomicPostResult,
    Values(PostCase{true, std::nullopt, true}, PostCase{false, true, true}, PostCase{false, false, false}),
    [](TestParamInfo<PostCase> const& info)
    {
        if (info.param.flip_ok)
        {
            return "page_flip_succeeds";
        }
        return info.param.crtc_ok.value_or(false) ? "set_crtc_recovers" : "presentation_fails";
    });

TEST_F(AtomicDisplaySinkTest, post_is_a_noop_without_a_queued_frame)
{
    EXPECT_CALL(*output, page_flip(_))
        .Times(0);
    EXPECT_CALL(*output, set_crtc(_))
        .Times(0);

    EXPECT_TRUE(sink->post());
}

TEST_F(AtomicDisplaySinkTest, successful_replacement_retires_the_previous_frame_during_post)
{
    auto frame_a = std::make_shared<NiceMock<mt::MockKMSFramebuffer>>();
    auto frame_b = std::make_shared<NiceMock<mt::MockKMSFramebuffer>>();

    MockFunction<void(std::string_view)> checkpoint;
    InSequence sequence;
    EXPECT_CALL(*output, page_flip(Ref(*frame_a)))
        .WillOnce(Return(true));
    EXPECT_CALL(*output, page_flip(Ref(*frame_b)))
        .WillOnce(Return(true));
    EXPECT_CALL(*frame_a, destroyed())
        .Times(1);
    EXPECT_CALL(checkpoint, Call("second post complete"));
    EXPECT_CALL(*frame_b, destroyed())
        .Times(1);

    ASSERT_TRUE(sink->overlay({fullscreen_element(frame_a)}));
    frame_a.reset();
    ASSERT_TRUE(sink->post());

    ASSERT_TRUE(sink->overlay({fullscreen_element(frame_b)}));
    frame_b.reset();
    ASSERT_TRUE(sink->post());
    checkpoint.Call("second post complete");

    sink.reset();
}

TEST_F(AtomicDisplaySinkTest, failed_presentation_retires_only_the_failed_frame)
{
    auto frame_a = std::make_shared<NiceMock<mt::MockKMSFramebuffer>>();
    auto frame_b = std::make_shared<NiceMock<mt::MockKMSFramebuffer>>();

    MockFunction<void(std::string_view)> checkpoint;
    InSequence sequence;
    EXPECT_CALL(*output, page_flip(Ref(*frame_a)))
        .WillOnce(Return(true));
    EXPECT_CALL(*output, page_flip(Ref(*frame_b)))
        .WillOnce(Return(false));
    EXPECT_CALL(*output, set_crtc(Ref(*frame_b)))
        .WillOnce(Return(false));
    EXPECT_CALL(*frame_b, destroyed())
        .Times(1);
    EXPECT_CALL(checkpoint, Call("failed post complete"));
    EXPECT_CALL(*frame_a, destroyed())
        .Times(1);

    ASSERT_TRUE(sink->overlay({fullscreen_element(frame_a)}));
    frame_a.reset();
    ASSERT_TRUE(sink->post());

    ASSERT_TRUE(sink->overlay({fullscreen_element(frame_b)}));
    frame_b.reset();
    EXPECT_FALSE(sink->post());
    checkpoint.Call("failed post complete");

    sink.reset();
}

TEST_F(AtomicDisplaySinkTest, replacing_a_queued_frame_retires_only_the_replaced_frame)
{
    auto frame_a = std::make_shared<NiceMock<mt::MockKMSFramebuffer>>();
    auto frame_b = std::make_shared<NiceMock<mt::MockKMSFramebuffer>>();
    auto const frame_c = std::make_shared<NiceMock<mt::MockKMSFramebuffer>>();

    MockFunction<void(std::string_view)> checkpoint;
    InSequence sequence;
    EXPECT_CALL(*output, page_flip(Ref(*frame_a)))
        .WillOnce(Return(true));
    EXPECT_CALL(*frame_b, destroyed())
        .Times(1);
    EXPECT_CALL(checkpoint, Call("queued frame replaced"));
    EXPECT_CALL(*frame_a, destroyed())
        .Times(1);

    ASSERT_TRUE(sink->overlay({fullscreen_element(frame_a)}));
    frame_a.reset();
    ASSERT_TRUE(sink->post());

    ASSERT_TRUE(sink->overlay({fullscreen_element(frame_b)}));
    frame_b.reset();
    ASSERT_TRUE(sink->overlay({fullscreen_element(frame_c)}));
    checkpoint.Call("queued frame replaced");

    sink.reset();
}

TEST_F(AtomicDisplaySinkTest, overlay_must_be_a_single_fullscreen_element)
{
    auto const framebuffer = std::make_shared<NiceMock<mt::MockKMSFramebuffer>>();
    auto element = fullscreen_element(framebuffer);

    EXPECT_TRUE(sink->overlay({element}));

    element.screen_positon.size = {20, 20};
    EXPECT_FALSE(sink->overlay({element}));
}
