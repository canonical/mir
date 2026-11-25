/*
 * Copyright © Canonical Ltd.
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
 */
#include <mir/graphics/texture.h>
#include <mir/test/doubles/null_emergency_cleanup.h>
#include "src/server/report/null_report_factory.h"
#include "src/platforms/gbm-kms/server/kms/platform.h"
#include "src/platforms/gbm-kms/server/kms/display_sink.h"
#include <mir/graphics/dmabuf_buffer.h>
#include <mir/test/doubles/mock_egl.h>
#include <mir/test/doubles/mock_gl.h>
#include <mir/test/doubles/mock_drm.h>
#include <mir/test/doubles/mock_buffer.h>
#include <mir/test/doubles/mock_gbm.h>
#include <mir/test/doubles/stub_gl_config.h>
#include <mir_test_framework/udev_environment.h>
#include <mir/test/doubles/fake_renderable.h>
#include <mir/graphics/transformation.h>
#include "mock_kms_output.h"

#include <fcntl.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <gbm.h>
#include <optional>

using namespace testing;
using namespace mir;
using namespace std;
using namespace mir::test;
using namespace mir::test::doubles;
using namespace mir_test_framework;
using namespace mir::graphics;
using namespace mir::graphics::gbm;
using mir::report::null_display_report;

namespace
{
class MockKMSFramebuffer : public FBHandle
{
public:
    MOCK_METHOD(uint32_t, operator_uint32_thunk, (), (const override));

    operator uint32_t() const override
    {
        return operator_uint32_thunk();
    }

    MOCK_METHOD(mir::geometry::Size, size, (), (const override));
};
}

class MesaDisplaySinkTest : public Test
{
public:
    int const mock_refresh_rate = 60;

    MesaDisplaySinkTest()
        : identity(1)
        , framebuffer{std::make_shared<NiceMock<MockKMSFramebuffer>>()}
        , fullscreen_list{
            mir::graphics::DisplayElement{
                display_area,
                {
                    {0, 0},
                    {display_area.size.width.as_value(), display_area.size.height.as_value()}
                },
                framebuffer}
            }
        , gbm{nullptr}
    {
        ON_CALL(mock_egl, eglChooseConfig(_,_,_,1,_))
            .WillByDefault(DoAll(SetArgPointee<2>(mock_egl.fake_configs[0]),
                                 SetArgPointee<4>(1),
                                 Return(EGL_TRUE)));

        mock_egl.provide_egl_extensions();
        mock_gl.provide_gles_extensions();

        ON_CALL(mock_gbm, gbm_bo_get_stride(_))
            .WillByDefault(Return(456));

        fake_devices.add_standard_device("standard-drm-devices");
        drm_fd = mir::Fd{::open("/dev/dri/card0", O_RDWR | O_CLOEXEC)};

        mock_kms_output = std::make_shared<NiceMock<MockKMSOutput>>();
        ON_CALL(*mock_kms_output, set_crtc_thunk(_))
            .WillByDefault(Return(true));
        ON_CALL(*mock_kms_output, schedule_page_flip_thunk(_))
            .WillByDefault(Return(true));
        ON_CALL(*mock_kms_output, max_refresh_rate())
            .WillByDefault(Return(mock_refresh_rate));

        ON_CALL(mock_drm, drmModeAddFB2WithModifiers(_,_,_,_,_,_,_,_,_,_))
            .WillByDefault(Return(0));
    }

protected:
    int const width{56};
    int const height{78};
    mir::geometry::Rectangle const display_area{{12,34}, {width,height}};
    glm::mat2 const identity;
    mir::Fd drm_fd;
    NiceMock<MockGBM> mock_gbm;
    NiceMock<MockEGL> mock_egl;
    NiceMock<MockGL>  mock_gl;
    NiceMock<MockDRM> mock_drm;
    UdevEnvironment   fake_devices;
    std::shared_ptr<MockKMSOutput> mock_kms_output;
    StubGLConfig gl_config;
    std::shared_ptr<mir::graphics::Framebuffer> const framebuffer;
    std::vector<mir::graphics::DisplayElement> const fullscreen_list;
    std::shared_ptr<struct gbm_device> const gbm;
};

TEST_F(MesaDisplaySinkTest, unrotated_view_area_is_untouched)
{
    graphics::gbm::DisplaySink sink(
        drm_fd,
        gbm,
        null_display_report(),
        {mock_kms_output},
        display_area,
        identity);

    EXPECT_EQ(display_area, sink.view_area());
}

TEST_F(MesaDisplaySinkTest, frames_requiring_gl_are_not_throttled)
{
    std::vector<mir::graphics::DisplayElement> const non_fullscreen_list{
        {
            {{12, 34}, {1, 1}},
            {{12, 34}, {1, 1}},
            nullptr
        }
    };

    graphics::gbm::DisplaySink sink(
        drm_fd,
        gbm,
        null_display_report(),
        {mock_kms_output},
        display_area,
        identity);

    for (int frame = 0; frame < 5; ++frame)
    {
        ASSERT_FALSE(sink.overlay(non_fullscreen_list));
        sink.post();

        // Cast to a simple int type so that test failures are readable
        ASSERT_EQ(0, sink.recommended_sleep().count());
    }
}

TEST_F(MesaDisplaySinkTest, untransformed_with_fullscreen_list_is_overlay)
{
    graphics::gbm::DisplaySink sink(
        drm_fd,
        gbm,
        null_display_report(),
        {mock_kms_output},
        display_area,
        identity);

    EXPECT_TRUE(sink.overlay(fullscreen_list));
}

TEST_F(MesaDisplaySinkTest, failed_post_keeps_the_previously_visible_framebuffer_alive)
{
    graphics::gbm::DisplaySink sink(
        drm_fd,
        gbm,
        null_display_report(),
        {mock_kms_output},
        display_area,
        identity);

    auto fullscreen_element = [this](std::shared_ptr<MockKMSFramebuffer> const& framebuffer)
    {
        return mir::graphics::DisplayElement{
            display_area,
            {{0, 0}, {display_area.size.width.as_value(), display_area.size.height.as_value()}},
            framebuffer};
    };

    auto frame_a = std::make_shared<NiceMock<MockKMSFramebuffer>>();
    auto frame_b = std::make_shared<NiceMock<MockKMSFramebuffer>>();
    auto frame_c = std::make_shared<NiceMock<MockKMSFramebuffer>>();
    std::weak_ptr<mir::graphics::Framebuffer> const weak_a{frame_a};
    std::weak_ptr<mir::graphics::Framebuffer> const weak_b{frame_b};

    InSequence sequence;
    EXPECT_CALL(*mock_kms_output, schedule_page_flip_thunk(_))
        .WillOnce(Return(true));
    EXPECT_CALL(*mock_kms_output, wait_for_page_flip())
        .Times(1);

    ASSERT_TRUE(sink.overlay({fullscreen_element(frame_a)}));
    frame_a.reset();
    ASSERT_TRUE(sink.post());
    EXPECT_FALSE(weak_a.expired());

    EXPECT_CALL(*mock_kms_output, schedule_page_flip_thunk(_))
        .WillOnce(Return(false));
    EXPECT_CALL(*mock_kms_output, set_crtc_thunk(_))
        .WillOnce(Return(false));

    ASSERT_TRUE(sink.overlay({fullscreen_element(frame_b)}));
    frame_b.reset();
    EXPECT_FALSE(sink.post());
    EXPECT_FALSE(weak_a.expired());
    EXPECT_TRUE(weak_b.expired());

    Mock::VerifyAndClearExpectations(mock_kms_output.get());

    ASSERT_TRUE(sink.overlay({fullscreen_element(frame_c)}));
    EXPECT_TRUE(sink.post());
    EXPECT_TRUE(weak_a.expired());
}

namespace
{
struct PostCase
{
    bool flip_ok;
    std::optional<bool> crtc_ok;
    bool expected;
    int wait_count;
};

class MesaPostResult : public MesaDisplaySinkTest, public WithParamInterface<PostCase>
{
};
}

TEST_P(MesaPostResult, post_reports_presentation_result)
{
    auto const& param = GetParam();
    graphics::gbm::DisplaySink sink(
        drm_fd,
        gbm,
        null_display_report(),
        mock_kms_output,
        display_area,
        identity);
    ASSERT_TRUE(sink.overlay(fullscreen_list));

    if (!param.crtc_ok.has_value())
    {
        EXPECT_CALL(*mock_kms_output, set_crtc_thunk(_))
            .Times(0);
    }

    InSequence sequence;
    EXPECT_CALL(*mock_kms_output, schedule_page_flip_thunk(Eq(framebuffer.get())))
        .WillOnce(Return(param.flip_ok));
    if (param.crtc_ok.has_value())
    {
        EXPECT_CALL(*mock_kms_output, set_crtc_thunk(Eq(framebuffer.get())))
            .WillOnce(Return(*param.crtc_ok));
    }
    EXPECT_CALL(*mock_kms_output, wait_for_page_flip())
        .Times(param.wait_count);

    EXPECT_EQ(param.expected, sink.post());
}

INSTANTIATE_TEST_SUITE_P(
    Presentation,
    MesaPostResult,
    Values(PostCase{true, std::nullopt, true, 1}, PostCase{false, false, false, 0}, PostCase{false, true, true, 0}),
    [](TestParamInfo<PostCase> const& info)
    {
        if (info.param.flip_ok)
        {
            return "page_flip_succeeds";
        }
        return info.param.crtc_ok.value_or(false) ? "set_crtc_recovers" : "presentation_fails";
    });

namespace
{
template<typename T>
auto fake_shared_ptr(intptr_t value) -> std::shared_ptr<T>
{
    return std::shared_ptr<T>{reinterpret_cast<T*>(value), [](auto) {}};
}
}

TEST_F(MesaDisplaySinkTest, transformation_not_implemented_internally)
{
    glm::mat2 const rotate_left = transformation(mir_orientation_left);

    graphics::gbm::DisplaySink sink(
        drm_fd,
        gbm,
        null_display_report(),
        {mock_kms_output},
        display_area,
        rotate_left);

    EXPECT_EQ(rotate_left, sink.transformation());
}
