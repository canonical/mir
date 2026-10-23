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
#include "mock_kms_output.h"

#include <mir/graphics/dmabuf_buffer.h>
#include <mir/graphics/drm_formats.h>
#include <mir/test/doubles/mock_drm.h>
#include <mir/test/doubles/mock_gbm.h>
#include <mir_test_framework/udev_environment.h>

#include <drm_fourcc.h>
#include <fcntl.h>
#include <gbm.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdint>

#include <memory>
#include <optional>
#include <span>

#include <vector>

namespace mg = mir::graphics;
namespace mga = mir::graphics::atomic;
namespace geom = mir::geometry;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

using namespace testing;

namespace
{
class MockKMSFramebuffer : public mg::FBHandle
{
public:
    operator uint32_t() const override
    {
        return operator_uint32_thunk();
    }

    MOCK_METHOD(uint32_t, operator_uint32_thunk, (), (const));
    MOCK_METHOD(geom::Size, size, (), (const, override));
};

class MockDMABufBuffer : public mg::DMABufBuffer
{
public:
    MOCK_METHOD(mg::DRMFormat, format, (), (const, override));
    MOCK_METHOD(std::optional<uint64_t>, modifier, (), (const, override));
    MOCK_METHOD(std::vector<PlaneDescriptor> const&, planes, (), (const, override));
    MOCK_METHOD(geom::Size, size, (), (const, override));
    MOCK_METHOD(mg::gl::Texture::Layout, layout, (), (const, override));
    MOCK_METHOD(void, on_consumed, (), (override));
};

struct ExpectedGbmImport
{
    uint32_t width;
    uint32_t height;
    uint32_t format;
    uint32_t num_fds;
    std::array<int, 4> fds;
    std::array<int, 4> strides;
    std::array<int, 4> offsets;
    uint64_t modifier;
};

MATCHER_P(IsGbmImportData, expected, "")
{
    auto const* data = static_cast<gbm_import_fd_modifier_data const*>(arg);
    if (!data)
    {
        return false;
    }
    if (data->width != expected.width ||
        data->height != expected.height ||
        data->format != expected.format ||
        data->num_fds != expected.num_fds ||
        data->modifier != expected.modifier)
    {
        return false;
    }
    for (auto i = 0u; i < expected.num_fds; ++i)
    {
        if (data->fds[i] != expected.fds[i] ||
            data->strides[i] != expected.strides[i] ||
            data->offsets[i] != expected.offsets[i])
        {
            return false;
        }
    }
    return true;
}

MATCHER_P(PlaneArrayIs, expected, "")
{
    auto const actual = std::span{arg, 4};
    return std::ranges::equal(actual, expected);
}

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

class ExplicitDmaBufImportTest : public AtomicDisplaySinkTest
{
protected:
    void SetUp() override
    {
        AtomicDisplaySinkTest::SetUp();

        ON_CALL(gbm, gbm_bo_import(_, _, _, _))
            .WillByDefault(Return(gbm.fake_gbm.bo));
        ON_CALL(drm, drmModeAddFB2WithModifiers(_, _, _, _, _, _, _, _, _, _))
            .WillByDefault(DoAll(SetArgPointee<8>(fb_id), Return(0)));
    }

    auto make_buffer(
        std::vector<mg::DMABufBuffer::PlaneDescriptor> const& planes,
        std::optional<uint64_t> modifier,
        geom::Size size = {64, 48}) -> std::shared_ptr<NiceMock<MockDMABufBuffer>>
    {
        auto buffer = std::make_shared<NiceMock<MockDMABufBuffer>>();
        ON_CALL(*buffer, planes()).WillByDefault(ReturnRef(planes));
        ON_CALL(*buffer, modifier()).WillByDefault(Return(modifier));
        ON_CALL(*buffer, size()).WillByDefault(Return(size));
        ON_CALL(*buffer, format()).WillByDefault(Return(mg::DRMFormat{DRM_FORMAT_NV12}));
        return buffer;
    }

    auto nv12_planes(uint32_t width = 64, uint32_t height = 48) const
        -> std::vector<mg::DMABufBuffer::PlaneDescriptor>
    {
        return {
            {mir::Fd{mir::IntOwnedFd{100}}, width, 0},
            {mir::Fd{mir::IntOwnedFd{101}}, width, width * height}};
    }

    auto gbm_device_ptr() const -> std::shared_ptr<struct gbm_device>
    {
        return {gbm.fake_gbm.device, [](struct gbm_device*) {}};
    }

    NiceMock<mtd::MockGBM> gbm;
    uint32_t const fb_id{0xabcd};
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

class ExplicitModifierPassThroughTest : public ExplicitDmaBufImportTest, public WithParamInterface<uint64_t>
{
};

class ExplicitModifierRejectionTest
    : public ExplicitDmaBufImportTest
    , public WithParamInterface<std::optional<uint64_t>>
{
};
}

TEST_P(AtomicPostResult, post_reports_presentation_result)
{
    auto const& param = GetParam();
    auto const framebuffer = std::make_shared<NiceMock<MockKMSFramebuffer>>();
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

TEST_F(AtomicDisplaySinkTest, failed_post_keeps_the_previously_visible_framebuffer_alive)
{
    auto frame_a = std::make_shared<NiceMock<MockKMSFramebuffer>>();
    auto frame_b = std::make_shared<NiceMock<MockKMSFramebuffer>>();
    auto frame_c = std::make_shared<NiceMock<MockKMSFramebuffer>>();
    std::weak_ptr<mg::Framebuffer> const weak_a{frame_a};
    std::weak_ptr<mg::Framebuffer> const weak_b{frame_b};

    InSequence sequence;
    EXPECT_CALL(*output, page_flip(Ref(*frame_a)))
        .WillOnce(Return(true));
    EXPECT_CALL(*output, page_flip(Ref(*frame_b)))
        .WillOnce(Return(false));
    EXPECT_CALL(*output, set_crtc(Ref(*frame_b)))
        .WillOnce(Return(false));
    EXPECT_CALL(*output, page_flip(Ref(*frame_c)))
        .WillOnce(Return(true));

    ASSERT_TRUE(sink->overlay({fullscreen_element(frame_a)}));
    frame_a.reset();
    ASSERT_TRUE(sink->post());
    EXPECT_FALSE(weak_a.expired());

    ASSERT_TRUE(sink->overlay({fullscreen_element(frame_b)}));
    frame_b.reset();
    EXPECT_FALSE(sink->post());
    EXPECT_FALSE(weak_a.expired());
    EXPECT_TRUE(weak_b.expired());
    EXPECT_TRUE(sink->post());

    ASSERT_TRUE(sink->overlay({fullscreen_element(frame_c)}));
    EXPECT_TRUE(sink->post());
    EXPECT_TRUE(weak_a.expired());
}

TEST_F(AtomicDisplaySinkTest, overlay_must_be_a_single_fullscreen_element)
{
    auto const framebuffer = std::make_shared<NiceMock<MockKMSFramebuffer>>();
    auto element = fullscreen_element(framebuffer);

    EXPECT_TRUE(sink->overlay({element}));

    element.screen_positon.size = {20, 20};
    EXPECT_FALSE(sink->overlay({element}));
}

TEST_P(ExplicitModifierPassThroughTest, explicit_modifier_is_passed_through_unchanged)
{
    auto const modifier = GetParam();

    std::vector<mg::DMABufBuffer::PlaneDescriptor> const planes{
        {mir::Fd{mir::IntOwnedFd{100}}, 64, 0}};
    auto const buffer = make_buffer(planes, modifier);

    ExpectedGbmImport const expected_import{
        64, 48, DRM_FORMAT_NV12, 1, {100, 0, 0, 0}, {64, 0, 0, 0}, {0, 0, 0, 0}, modifier};
    auto const expected_handles = std::array<uint32_t, 4>{0x0987, 0, 0, 0};
    auto const expected_pitches = std::array<uint32_t, 4>{64, 0, 0, 0};
    auto const expected_offsets = std::array<uint32_t, 4>{0, 0, 0, 0};
    auto const expected_modifiers = std::array<uint64_t, 4>{modifier, 0, 0, 0};

    EXPECT_CALL(gbm, gbm_bo_import(_, GBM_BO_IMPORT_FD_MODIFIER, IsGbmImportData(expected_import), GBM_BO_USE_SCANOUT))
        .WillOnce(Return(gbm.fake_gbm.bo));
    EXPECT_CALL(drm, drmModeAddFB2WithModifiers(
        Eq(static_cast<int>(drm_fd)), 64u, 48u, DRM_FORMAT_NV12,
        PlaneArrayIs(expected_handles),
        PlaneArrayIs(expected_pitches),
        PlaneArrayIs(expected_offsets),
        PlaneArrayIs(expected_modifiers),
        _, DRM_MODE_FB_MODIFIERS))
        .WillOnce(DoAll(SetArgPointee<8>(fb_id), Return(0)));

    auto const result = mga::import_explicit_dmabuf_framebuffer(drm_fd, gbm_device_ptr(), buffer);

    ASSERT_NE(result, nullptr);
    EXPECT_EQ(static_cast<uint32_t>(*result), fb_id);
}

INSTANTIATE_TEST_SUITE_P(
    ModifierPassThrough,
    ExplicitModifierPassThroughTest,
    Values(uint64_t{DRM_FORMAT_MOD_LINEAR}, uint64_t{DRM_FORMAT_MOD_SAMSUNG_64_32_TILE}),
    [](TestParamInfo<uint64_t> const& info)
    {
        return info.param == DRM_FORMAT_MOD_LINEAR ? "linear" : "nonlinear";
    });

TEST_P(ExplicitModifierRejectionTest, missing_or_invalid_modifier_is_rejected)
{
    auto const buffer = make_buffer(nv12_planes(), GetParam());

    EXPECT_CALL(gbm, gbm_bo_import(_, _, _, _)).Times(0);
    EXPECT_CALL(drm, drmModeAddFB2WithModifiers(_, _, _, _, _, _, _, _, _, _)).Times(0);

    EXPECT_EQ(mga::import_explicit_dmabuf_framebuffer(drm_fd, gbm_device_ptr(), buffer), nullptr);
}

INSTANTIATE_TEST_SUITE_P(
    ModifierRejection,
    ExplicitModifierRejectionTest,
    Values(std::optional<uint64_t>{}, std::optional<uint64_t>{static_cast<uint64_t>(DRM_FORMAT_MOD_INVALID)}),
    [](TestParamInfo<std::optional<uint64_t>> const& info)
    {
        return info.param.has_value() ? "invalid" : "absent";
    });

TEST_F(ExplicitDmaBufImportTest, two_plane_nv12_metadata_is_forwarded_exactly)
{
    auto const width = 64u;
    auto const height = 48u;
    auto const second_plane_offset = width * height;
    auto const second_plane_stride = width * 2;

    std::vector<mg::DMABufBuffer::PlaneDescriptor> const planes{
        {mir::Fd{mir::IntOwnedFd{100}}, width, 0},
        {mir::Fd{mir::IntOwnedFd{101}}, second_plane_stride, second_plane_offset}};
    auto const buffer = make_buffer(planes, DRM_FORMAT_MOD_SAMSUNG_64_32_TILE);

    gbm_bo_handle handle0{};
    handle0.u32 = 0x1111;
    gbm_bo_handle handle1{};
    handle1.u32 = 0x2222;
    ON_CALL(gbm, gbm_bo_get_handle_for_plane(_, 0)).WillByDefault(Return(handle0));
    ON_CALL(gbm, gbm_bo_get_handle_for_plane(_, 1)).WillByDefault(Return(handle1));

    ExpectedGbmImport const expected_import{
        width, height, DRM_FORMAT_NV12, 2,
        {100, 101, 0, 0},
        {static_cast<int>(width), static_cast<int>(second_plane_stride), 0, 0},
        {0, static_cast<int>(second_plane_offset), 0, 0},
        DRM_FORMAT_MOD_SAMSUNG_64_32_TILE};
    auto const expected_handles = std::array<uint32_t, 4>{0x1111, 0x2222, 0, 0};
    auto const expected_pitches = std::array<uint32_t, 4>{width, second_plane_stride, 0, 0};
    auto const expected_offsets = std::array<uint32_t, 4>{0, second_plane_offset, 0, 0};
    auto const expected_modifiers = std::array<uint64_t, 4>{
        DRM_FORMAT_MOD_SAMSUNG_64_32_TILE, DRM_FORMAT_MOD_SAMSUNG_64_32_TILE, 0, 0};

    EXPECT_CALL(gbm, gbm_bo_import(_, GBM_BO_IMPORT_FD_MODIFIER, IsGbmImportData(expected_import), GBM_BO_USE_SCANOUT))
        .WillOnce(Return(gbm.fake_gbm.bo));
    EXPECT_CALL(drm, drmModeAddFB2WithModifiers(
        Eq(static_cast<int>(drm_fd)), width, height, DRM_FORMAT_NV12,
        PlaneArrayIs(expected_handles),
        PlaneArrayIs(expected_pitches),
        PlaneArrayIs(expected_offsets),
        PlaneArrayIs(expected_modifiers),
        _, DRM_MODE_FB_MODIFIERS))
        .WillOnce(DoAll(SetArgPointee<8>(fb_id), Return(0)));

    auto const result = mga::import_explicit_dmabuf_framebuffer(drm_fd, gbm_device_ptr(), buffer);

    ASSERT_NE(result, nullptr);
    EXPECT_EQ(static_cast<uint32_t>(*result), fb_id);
}

TEST_F(ExplicitDmaBufImportTest, gbm_import_failure_is_rejected)
{
    std::vector<mg::DMABufBuffer::PlaneDescriptor> const planes{
        {mir::Fd{mir::IntOwnedFd{100}}, 64, 0}};
    auto const buffer = make_buffer(planes, DRM_FORMAT_MOD_LINEAR);

    EXPECT_CALL(gbm, gbm_bo_import(_, _, _, _))
        .WillOnce(SetErrnoAndReturn(EINVAL, nullptr));
    EXPECT_CALL(drm, drmModeAddFB2WithModifiers(_, _, _, _, _, _, _, _, _, _)).Times(0);

    EXPECT_EQ(mga::import_explicit_dmabuf_framebuffer(drm_fd, gbm_device_ptr(), buffer), nullptr);
}

TEST_F(ExplicitDmaBufImportTest, add_fb_failure_destroys_the_bo_and_drops_the_buffer)
{
    std::vector<mg::DMABufBuffer::PlaneDescriptor> const planes{
        {mir::Fd{mir::IntOwnedFd{100}}, 64, 0}};
    auto buffer = make_buffer(planes, DRM_FORMAT_MOD_LINEAR);
    std::weak_ptr<mg::DMABufBuffer> const weak_buffer{buffer};

    EXPECT_CALL(drm, drmModeAddFB2WithModifiers(_, _, _, _, _, _, _, _, _, _))
        .WillOnce(Return(-EINVAL));
    EXPECT_CALL(gbm, gbm_bo_destroy(gbm.fake_gbm.bo)).Times(1);
    EXPECT_CALL(drm, drmModeRmFB(_, _)).Times(0);

    EXPECT_EQ(mga::import_explicit_dmabuf_framebuffer(drm_fd, gbm_device_ptr(), buffer), nullptr);

    buffer.reset();
    EXPECT_TRUE(weak_buffer.expired());
}

TEST_F(ExplicitDmaBufImportTest, handle_owns_the_bo_and_the_buffer_until_destroyed)
{
    std::vector<mg::DMABufBuffer::PlaneDescriptor> const planes{
        {mir::Fd{mir::IntOwnedFd{100}}, 64, 0}};
    auto buffer = make_buffer(planes, DRM_FORMAT_MOD_LINEAR);
    std::weak_ptr<mg::DMABufBuffer> const weak_buffer{buffer};

    EXPECT_CALL(gbm, gbm_bo_destroy(gbm.fake_gbm.bo)).Times(0);
    EXPECT_CALL(drm, drmModeRmFB(_, _)).Times(0);

    auto result = mga::import_explicit_dmabuf_framebuffer(drm_fd, gbm_device_ptr(), buffer);
    ASSERT_NE(result, nullptr);

    buffer.reset();
    EXPECT_FALSE(weak_buffer.expired());

    Mock::VerifyAndClearExpectations(&gbm);
    Mock::VerifyAndClearExpectations(&drm);

    EXPECT_CALL(gbm, gbm_bo_destroy(gbm.fake_gbm.bo)).Times(1);
    EXPECT_CALL(drm, drmModeRmFB(_, fb_id)).Times(1);

    result.reset();
    EXPECT_TRUE(weak_buffer.expired());
}

TEST_F(ExplicitDmaBufImportTest, does_not_call_on_consumed)
{
    std::vector<mg::DMABufBuffer::PlaneDescriptor> const planes{
        {mir::Fd{mir::IntOwnedFd{100}}, 64, 0}};
    auto const buffer = make_buffer(planes, DRM_FORMAT_MOD_LINEAR);

    EXPECT_CALL(*buffer, on_consumed()).Times(0);

    auto const result = mga::import_explicit_dmabuf_framebuffer(drm_fd, gbm_device_ptr(), buffer);

    ASSERT_NE(result, nullptr);
}
