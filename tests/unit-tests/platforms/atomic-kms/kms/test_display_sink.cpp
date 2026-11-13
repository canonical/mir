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
#include "src/platforms/atomic-kms/server/kms/plane_capabilities.h"
#include "src/platforms/atomic-kms/server/kms/quirks.h"
#include "src/server/report/null_report_factory.h"
#include "mock_kms_framebuffer.h"
#include "mock_kms_output.h"

#include <mir/graphics/dmabuf_buffer.h>
#include <mir/graphics/drm_formats.h>
#include <mir/graphics/texture.h>
#include <mir/graphics/transformation.h>
#include <mir/test/doubles/mock_buffer.h>
#include <mir/test/doubles/mock_drm.h>
#include <mir/test/doubles/mock_gbm.h>
#include <mir/test/doubles/mock_renderable.h>
#include <mir_test_framework/udev_environment.h>

#include <drm_fourcc.h>
#include <fcntl.h>
#include <gbm.h>
#include <glm/gtc/matrix_transform.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cerrno>
#include <cstdint>

#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mg = mir::graphics;
namespace mga = mir::graphics::atomic;
namespace geom = mir::geometry;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

using namespace testing;

namespace
{
geom::Size const output_size{56, 78};
geom::Rectangle const display_area{{12, 34}, output_size};
uint64_t const supported_modifier{DRM_FORMAT_MOD_LINEAR};
uint64_t const other_modifier{0x0100000000000002ULL};

uint32_t const first_overlay_id{50};
uint32_t const second_overlay_id{51};
uint32_t const client_fb_id{0x1234};
geom::Size const client_size{20, 10};
geom::Point const client_top_left{20, 40};
// Starts left of and above the output, so the client is only partly on-screen.
geom::Point const client_top_left_outside_output{
    display_area.top_left.x - geom::DeltaX{client_size.width.as_int() / 2},
    display_area.top_left.y - geom::DeltaY{client_size.height.as_int() / 2}};
// Entirely to the right of the output, so the client is not on-screen at all.
geom::Point const client_top_left_clear_of_output{
    display_area.top_left + geom::DeltaX{display_area.size.width.as_int()}};

auto make_client_framebuffer(
    mga::FormatModifier const& format_modifier = {DRM_FORMAT_NV12, supported_modifier},
    mg::gl::Texture::Layout layout = mg::gl::Texture::Layout::GL)
    -> std::shared_ptr<mga::DirectScanoutFramebuffer>
{
    return std::make_shared<mga::DirectScanoutFramebuffer>(
        std::make_shared<uint32_t>(client_fb_id), client_size, format_modifier, layout);
}

mga::CrtcPlaneCapabilities legal_capabilities()
{
    auto overlay = mga::PlaneCapability{
        .id = {},
        .zpos = {},
        // XRGB8888 keeps the format-guard rejection row from also failing the plane lookup.
        .formats = {{DRM_FORMAT_NV12, supported_modifier}, {DRM_FORMAT_XRGB8888, supported_modifier}},
        .alpha_max = mga::PropertyValue{0xffff},
        .blend_none = {},
        .fixed_opaque = false,
        .bt709_limited = mga::Bt709Limited{1, 2},
        .can_be_above_primary = true};
    return mga::CrtcPlaneCapabilities{
        .crtc_id = {},
        .primary = {},
        .overlays = {std::move(overlay)}};
}

auto shared_capabilities(mga::CrtcPlaneCapabilities capabilities)
    -> std::shared_ptr<mga::CrtcPlaneCapabilities const>
{
    return std::make_shared<mga::CrtcPlaneCapabilities const>(std::move(capabilities));
}

struct StubDMABufBuffer : mg::DMABufBuffer
{
    explicit StubDMABufBuffer(geom::Size size) : size_{size} {}

    auto format() const -> mg::DRMFormat override { return format_; }
    auto modifier() const -> std::optional<uint64_t> override { return modifier_; }
    auto planes() const -> std::vector<PlaneDescriptor> const& override { return planes_; }
    auto layout() const -> mg::gl::Texture::Layout override { return layout_; }
    auto size() const -> geom::Size override { return size_; }

    mg::DRMFormat format_{DRM_FORMAT_NV12};
    std::optional<uint64_t> modifier_{supported_modifier};
    std::vector<PlaneDescriptor> planes_;
    mg::gl::Texture::Layout layout_{mg::gl::Texture::Layout::GL};
    geom::Size size_;
};

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
        ON_CALL(*output, size())
            .WillByDefault(Return(output_size));

        ON_CALL(*output, plane_capabilities())
            .WillByDefault(Return(shared_capabilities(legal_capabilities())));

        sink = make_sink();
    }

    auto make_sink(
        glm::mat2 const& transformation = glm::mat2{1},
        mga::PlanePromotion promotion = mga::PlanePromotion::available) -> std::unique_ptr<mga::DisplaySink>
    {
        return std::make_unique<mga::DisplaySink>(
            drm_fd,
            nullptr,
            mga::BypassOption::allowed,
            promotion,
            mir::report::null_display_report(),
            output,
            display_area,
            transformation,
            std::make_shared<mga::GbmQuirks>(std::make_unique<StubSurfaceHasFreeBuffersQuirk>()));
    }

    auto make_candidate(
        std::shared_ptr<StubDMABufBuffer> const& dmabuf,
        geom::Rectangle const& screen_position) const
        -> std::shared_ptr<NiceMock<mtd::MockRenderable>>
    {
        std::shared_ptr<NiceMock<mtd::MockBuffer>> buffer{
            new NiceMock<mtd::MockBuffer>(),
            [dmabuf](auto* buffer) { delete buffer; }};
        ON_CALL(*buffer, native_buffer_base())
            .WillByDefault(Return(dmabuf.get()));

        auto renderable = make_plain_renderable(screen_position);
        ON_CALL(*renderable, buffer())
            .WillByDefault(Return(buffer));
        ON_CALL(*renderable, src_bounds())
            .WillByDefault(Return(geom::RectangleD{{0, 0}, dmabuf->size_}));
        return renderable;
    }

    auto make_candidate(geom::Rectangle const& screen_position) const
        -> std::shared_ptr<NiceMock<mtd::MockRenderable>>
    {
        return make_candidate(
            std::make_shared<StubDMABufBuffer>(screen_position.size),
            screen_position);
    }

    auto make_plain_renderable(geom::Rectangle const& screen_position) const
        -> std::shared_ptr<NiceMock<mtd::MockRenderable>>
    {
        auto renderable = std::make_shared<NiceMock<mtd::MockRenderable>>();
        ON_CALL(*renderable, screen_position())
            .WillByDefault(Return(screen_position));
        ON_CALL(*renderable, transformation())
            .WillByDefault(Return(glm::mat4{1}));
        return renderable;
    }

    void expect_plan(
        std::optional<mg::LogicalStacking> const& stacking,
        mg::RenderableList const& renderables,
        std::size_t selected) const
    {
        ASSERT_TRUE(stacking);
        ASSERT_THAT(stacking->layers, SizeIs(2));
        auto const compositing = std::get_if<mg::CompositingLayer>(&stacking->layers[0]);
        auto const direct = std::get_if<mg::DirectLayer>(&stacking->layers[1]);
        ASSERT_THAT(compositing, NotNull());
        ASSERT_THAT(direct, NotNull());
        auto const expected = renderables
            | std::views::filter([&](auto const& renderable) { return renderable != renderables[selected]; })
            | std::ranges::to<std::vector>();
        EXPECT_THAT(compositing->renderables, ElementsAreArray(expected));
        EXPECT_EQ(direct->renderable, renderables[selected]);
    }

    auto fullscreen_element(std::shared_ptr<mg::Framebuffer> const& framebuffer) const -> mg::DisplayElement
    {
        return {
            display_area,
            {{0, 0}, {display_area.size.width.as_int(), display_area.size.height.as_int()}},
            framebuffer};
    }

    auto client_element(std::shared_ptr<mg::Framebuffer> const& framebuffer, geom::Point const& top_left) const
        -> mg::DisplayElement
    {
        return {
            {top_left, client_size},
            {{0, 0}, {client_size.width.as_value(), client_size.height.as_value()}},
            framebuffer};
    }

    mir_test_framework::UdevEnvironment devices;
    NiceMock<mtd::MockDRM> drm;
    mir::Fd drm_fd;
    std::shared_ptr<NiceMock<mt::MockKMSOutput>> const output{
        std::make_shared<NiceMock<mt::MockKMSOutput>>()};
    std::unique_ptr<mga::DisplaySink> sink;
};

class DmaBufImportTest : public Test
{
protected:
    void SetUp() override
    {
        ON_CALL(gbm, gbm_bo_import(_, _, _, _))
            .WillByDefault(Return(gbm.fake_gbm.bo));
        ON_CALL(drm, drmModeAddFB2WithModifiers(_, _, _, _, _, _, _, _, _, _))
            .WillByDefault(DoAll(SetArgPointee<8>(fb_id), Return(0)));
    }

    auto make_buffer() -> std::shared_ptr<StubDMABufBuffer>
    {
        auto buffer = std::make_shared<StubDMABufBuffer>(geom::Size{64, 48});
        buffer->planes_.push_back({mir::Fd{}, 64, 0});
        return buffer;
    }

    auto gbm_device_ptr() const -> std::shared_ptr<struct gbm_device>
    {
        return {gbm.fake_gbm.device, [](struct gbm_device*) {}};
    }

    NiceMock<mtd::MockDRM> drm;
    NiceMock<mtd::MockGBM> gbm;
    // MockDRM and MockGBM intercept every call, so no real fd is needed.
    mir::Fd const drm_fd;
    uint32_t const fb_id{0xabcd};
    mga::DmaBufDisplayAllocator allocator{gbm_device_ptr(), drm_fd};
};

struct ImportModifierCase
{
    std::string_view name;
    std::optional<uint64_t> buffer_modifier;
    uint64_t gbm_modifier;
    uint64_t addfb_modifier;
};

class DmaBufImportModifierTest : public DmaBufImportTest, public WithParamInterface<ImportModifierCase>
{
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

struct RejectionCase
{
    std::string_view name;
    std::optional<uint64_t> modifier{supported_modifier};
    uint32_t format{DRM_FORMAT_NV12};
    float alpha{1.0f};
    geom::Rectangle destination{display_area};
    glm::mat4 transformation{1};
    MirOrientation orientation{mir_orientation_normal};
    MirMirrorMode mirror{mir_mirror_mode_none};
    std::optional<geom::Rectangle> clip{};
    mg::gl::Texture::Layout layout{mg::gl::Texture::Layout::GL};
    geom::RectangleD source{{0, 0}, output_size};
};

class AtomicPlanRejection : public AtomicDisplaySinkTest, public WithParamInterface<RejectionCase>
{
};

struct IneligibleOutputCase
{
    std::string_view name;
    std::shared_ptr<mga::CrtcPlaneCapabilities const> capabilities{shared_capabilities(legal_capabilities())};
    glm::mat2 transformation{glm::mat2{1}};
    mga::PlanePromotion promotion{mga::PlanePromotion::available};
    geom::Size reported_output_size{output_size};
};

class AtomicPlanIneligibleOutput : public AtomicDisplaySinkTest, public WithParamInterface<IneligibleOutputCase>
{
};

struct PlaneCase
{
    std::string_view name;
    bool above_primary{true};
    bool bt709_limited{true};
    uint64_t modifier{supported_modifier};
    std::optional<mga::PropertyValue> alpha_max{mga::PropertyValue{0xffff}};
    bool append_legal_plane{false};
    bool expected{false};
};

class AtomicPlanPlaneLegality : public AtomicDisplaySinkTest, public WithParamInterface<PlaneCase>
{
};

struct SceneElement
{
    geom::Rectangle area;
    bool dmabuf{false};
    glm::mat4 transformation{1};
    uint64_t modifier{supported_modifier};
    MirOrientation orientation{mir_orientation_normal};
};

struct SceneCase
{
    std::string_view name;
    std::vector<SceneElement> elements;
    std::optional<std::size_t> selected;
};

class AtomicPlanScene : public AtomicDisplaySinkTest, public WithParamInterface<SceneCase>
{
};

struct MixedRejectionCase
{
    std::string_view name;
    std::shared_ptr<mga::CrtcPlaneCapabilities const> capabilities{shared_capabilities(legal_capabilities())};
    bool plain_client_framebuffer{false};
    uint32_t client_format{DRM_FORMAT_NV12};
    uint64_t client_modifier{supported_modifier};
    mg::gl::Texture::Layout client_layout{mg::gl::Texture::Layout::GL};
    geom::Rectangle primary_destination{display_area};
    geom::Rectangle client_destination{client_top_left, client_size};
    geom::RectangleF client_source{{0, 0}, {client_size.width.as_value(), client_size.height.as_value()}};
};

auto mixed_state_matcher(
    mg::FBHandle const* primary,
    mg::FBHandle const* client,
    mga::PlaneId plane,
    geom::Point destination) -> Matcher<mga::PresentationState const&>
{
    return AllOf(
        Property(&mga::PresentationState::primary, Ref(*primary)),
        Property(
            &mga::PresentationState::client_planes,
            ElementsAre(AllOf(
                Field(&mga::ClientPlaneAssignment::plane, plane),
                Field(&mga::ClientPlaneAssignment::framebuffer, Pointer(client)),
                Field(&mga::ClientPlaneAssignment::destination, destination)))));
}

struct MixedPresentationCase
{
    std::string_view name;
    bool kernel_accepts;
    geom::Point client_top_left;
};

class AtomicMixedPresentation : public AtomicDisplaySinkTest, public WithParamInterface<MixedPresentationCase>
{
};

class AtomicMixedRejection : public AtomicDisplaySinkTest, public WithParamInterface<MixedRejectionCase>
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

TEST_F(AtomicDisplaySinkTest, failed_post_keeps_the_previously_visible_framebuffer_alive)
{
    auto frame_a = std::make_shared<NiceMock<mt::MockKMSFramebuffer>>();
    auto frame_b = std::make_shared<NiceMock<mt::MockKMSFramebuffer>>();
    auto frame_c = std::make_shared<NiceMock<mt::MockKMSFramebuffer>>();
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

TEST_F(AtomicDisplaySinkTest, replacing_a_queued_frame_retires_only_the_replaced_frame)
{
    auto frame_a = std::make_shared<NiceMock<mt::MockKMSFramebuffer>>();
    auto frame_b = std::make_shared<NiceMock<mt::MockKMSFramebuffer>>();
    auto const frame_c = std::make_shared<NiceMock<mt::MockKMSFramebuffer>>();
    std::weak_ptr<mg::Framebuffer> const weak_a{frame_a};
    std::weak_ptr<mg::Framebuffer> const weak_b{frame_b};

    EXPECT_CALL(*output, page_flip(Ref(*frame_a)))
        .WillOnce(Return(true));

    ASSERT_TRUE(sink->overlay({fullscreen_element(frame_a)}));
    frame_a.reset();
    ASSERT_TRUE(sink->post());

    ASSERT_TRUE(sink->overlay({fullscreen_element(frame_b)}));
    frame_b.reset();
    ASSERT_TRUE(sink->overlay({fullscreen_element(frame_c)}));

    EXPECT_FALSE(weak_a.expired());
    EXPECT_TRUE(weak_b.expired());

    sink.reset();
    EXPECT_TRUE(weak_a.expired());
}

TEST_F(AtomicDisplaySinkTest, overlay_must_be_a_single_fullscreen_element)
{
    auto const framebuffer = std::make_shared<NiceMock<mt::MockKMSFramebuffer>>();
    auto element = fullscreen_element(framebuffer);

    EXPECT_TRUE(sink->overlay({element}));

    element.screen_positon.size = {20, 20};
    EXPECT_FALSE(sink->overlay({element}));
}

TEST_P(AtomicPlanRejection, plan_presentation_rejects_ineligible_candidate)
{
    auto dmabuf = std::make_shared<StubDMABufBuffer>(output_size);
    auto candidate = make_candidate(dmabuf, display_area);
    auto const& param = GetParam();
    dmabuf->modifier_ = param.modifier;
    dmabuf->format_ = mg::DRMFormat{param.format};
    ON_CALL(*candidate, alpha()).WillByDefault(Return(param.alpha));
    ON_CALL(*candidate, screen_position()).WillByDefault(Return(param.destination));
    ON_CALL(*candidate, transformation()).WillByDefault(Return(param.transformation));
    ON_CALL(*candidate, orientation()).WillByDefault(Return(param.orientation));
    ON_CALL(*candidate, mirror_mode()).WillByDefault(Return(param.mirror));
    ON_CALL(*candidate, clip_area()).WillByDefault(Return(param.clip));
    ON_CALL(*candidate, src_bounds()).WillByDefault(Return(param.source));
    dmabuf->layout_ = param.layout;
    auto capabilities = legal_capabilities();
    capabilities.overlays.front().formats.push_back({DRM_FORMAT_NV12, DRM_FORMAT_MOD_INVALID});
    ON_CALL(*output, plane_capabilities())
        .WillByDefault(Return(shared_capabilities(std::move(capabilities))));

    EXPECT_FALSE(sink->plan_presentation({candidate}).has_value());
}

INSTANTIATE_TEST_SUITE_P(
    CandidateEligibility,
    AtomicPlanRejection,
    Values(
        RejectionCase{.name = "missing_modifier", .modifier = std::nullopt},
        RejectionCase{.name = "wrong_format", .format = DRM_FORMAT_XRGB8888},
        RejectionCase{.name = "translucent", .alpha = 0.5f},
        RejectionCase{.name = "scaled_destination", .destination = {display_area.top_left, output_size / 2}},
        RejectionCase{.name = "extends_beyond_output",
                      .destination = {display_area.top_left + geom::DeltaX{1}, output_size}},
        RejectionCase{.name = "transformed_surface", .transformation = glm::mat4{2.0f}},
        RejectionCase{.name = "invalid_modifier", .modifier = DRM_FORMAT_MOD_INVALID},
        RejectionCase{.name = "orientation", .orientation = mir_orientation_left},
        RejectionCase{.name = "mirror", .mirror = mir_mirror_mode_horizontal},
        RejectionCase{.name = "clip", .clip = display_area},
        RejectionCase{.name = "y_inverted", .layout = mg::gl::Texture::Layout::TopRowFirst},
        RejectionCase{.name = "source_offset", .source = {{1, 0}, output_size}},
        RejectionCase{.name = "cropped_source", .source = {{0, 0}, output_size / 2}}),
    [](TestParamInfo<RejectionCase> const& info)
    {
        return std::string{info.param.name};
    });

TEST_F(AtomicDisplaySinkTest, shaped_opaque_format_candidate_is_promoted)
{
    auto dmabuf = std::make_shared<StubDMABufBuffer>(output_size);
    dmabuf->format_ = mg::DRMFormat{DRM_FORMAT_NV12};
    auto candidate = make_candidate(dmabuf, display_area);
    ON_CALL(*candidate, alpha()).WillByDefault(Return(1.0f));
    ON_CALL(*candidate, shaped()).WillByDefault(Return(true));

    expect_plan(sink->plan_presentation({candidate}), {candidate}, 0);
}

TEST_P(AtomicPlanIneligibleOutput, plan_presentation_requires_eligible_output)
{
    auto const& param = GetParam();
    ON_CALL(*output, size())
        .WillByDefault(Return(param.reported_output_size));
    ON_CALL(*output, plane_capabilities())
        .WillByDefault(Return(param.capabilities));
    sink = make_sink(param.transformation, param.promotion);

    auto candidate = make_candidate(display_area);

    EXPECT_FALSE(sink->plan_presentation({candidate}).has_value());
}

INSTANTIATE_TEST_SUITE_P(
    OutputEligibility,
    AtomicPlanIneligibleOutput,
    Values(
        IneligibleOutputCase{.name = "plane_promotion_unavailable", .promotion = mga::PlanePromotion::unavailable},
        IneligibleOutputCase{.name = "no_plane_capabilities", .capabilities = nullptr},
        IneligibleOutputCase{.name = "output_size_mismatch", .reported_output_size = geom::Size{55, 78}},
        IneligibleOutputCase{.name = "non_identity_output_transformation", .transformation = glm::mat2{2.0f}}),
    [](TestParamInfo<IneligibleOutputCase> const& info)
    {
        return std::string{info.param.name};
    });

TEST_P(AtomicPlanPlaneLegality, plan_presentation_requires_a_legal_plane)
{
    auto const& param = GetParam();
    auto capabilities = legal_capabilities();
    auto& plane = capabilities.overlays.front();
    plane.can_be_above_primary = param.above_primary;
    if (!param.bt709_limited)
        plane.bt709_limited = std::nullopt;
    plane.formats = {{DRM_FORMAT_NV12, param.modifier}};
    plane.alpha_max = param.alpha_max;
    if (param.append_legal_plane)
        capabilities.overlays.push_back(legal_capabilities().overlays.front());
    ON_CALL(*output, plane_capabilities())
        .WillByDefault(Return(shared_capabilities(std::move(capabilities))));

    auto candidate = make_candidate(display_area);

    auto const stacking = sink->plan_presentation({candidate});

    if (param.expected)
        expect_plan(stacking, {candidate}, 0);
    else
        EXPECT_FALSE(stacking);
}

INSTANTIATE_TEST_SUITE_P(
    PlaneLegality,
    AtomicPlanPlaneLegality,
    Values(
        PlaneCase{.name = "cannot_be_above_primary", .above_primary = false},
        PlaneCase{.name = "no_bt709_limited", .bt709_limited = false},
        PlaneCase{.name = "modifier_not_supported", .modifier = other_modifier},
        PlaneCase{.name = "partial_alpha_max", .alpha_max = mga::PropertyValue{0x7fff}},
        PlaneCase{.name = "opaque_alpha_max", .expected = true},
        PlaneCase{.name = "no_alpha_property", .alpha_max = std::nullopt, .expected = true},
        PlaneCase{.name = "legal_second_plane", .above_primary = false,
                  .append_legal_plane = true, .expected = true}),
    [](TestParamInfo<PlaneCase> const& info)
    {
        return std::string{info.param.name};
    });

TEST_P(AtomicPlanScene, selects_largest_unobstructed_candidate)
{
    auto const& param = GetParam();
    mg::RenderableList renderables;
    for (auto const& element : param.elements)
    {
        auto dmabuf = std::make_shared<StubDMABufBuffer>(element.area.size);
        dmabuf->modifier_ = element.modifier;
        auto renderable = element.dmabuf ? make_candidate(dmabuf, element.area) : make_plain_renderable(element.area);
        ON_CALL(*renderable, transformation()).WillByDefault(Return(element.transformation));
        ON_CALL(*renderable, orientation()).WillByDefault(Return(element.orientation));
        renderables.push_back(std::move(renderable));
    }

    auto const stacking = sink->plan_presentation(renderables);

    if (param.selected)
        expect_plan(stacking, renderables, *param.selected);
    else
        EXPECT_FALSE(stacking);
}

INSTANTIATE_TEST_SUITE_P(
    ScenePolicy, AtomicPlanScene,
    Values(
        SceneCase{"excludes_only_direct_candidate", {{{{12, 34}, {20, 20}}}, {display_area, true}}, 1},
        SceneCase{"largest_area_in_middle", {{{{12, 34}, {40, 6}}, true}, {{{12, 44}, {20, 20}}, true},
                                             {{{38, 44}, {6, 40}}, true}, {{{50, 90}, {10, 10}}}}, 1},
        SceneCase{"equal_area_prefers_frontmost", {{{{12, 34}, {20, 10}}, true},
                                                   {{{40, 34}, {10, 20}}, true}}, 1},
        SceneCase{"touching_edges_do_not_overlap", {{{{12, 34}, {30, 30}}, true},
                                                     {{{42, 34}, {10, 10}}}}, 0},
        SceneCase{"unsupported_modifier_falls_back", {{{{12, 34}, {30, 30}}, true, glm::mat4{1}, other_modifier},
                                                      {{{44, 34}, {20, 20}}, true}}, 1},
        SceneCase{"blocked_large_falls_back", {{{{12, 34}, {30, 30}}, true}, {{{12, 34}, {10, 10}}},
                                              {{{44, 34}, {20, 20}}, true}}, 2},
        SceneCase{"overlapping_plain_ui", {{display_area, true}, {{{12, 34}, {10, 10}}}}, std::nullopt},
        SceneCase{"w_scaling_transform_above", {{{{12, 34}, {30, 30}}, true},
                                                {{{50, 34}, {20, 20}}, false, glm::mat4{2.0f}}}, std::nullopt},
        SceneCase{"half_turn_above_disjoint", {{{{12, 34}, {30, 30}}, true},
            {{{50, 34}, {10, 10}}, false, glm::mat4{mg::transformation(mir_orientation_inverted)}}}, 0},
        SceneCase{"quarter_turn_above_disjoint", {{{{12, 34}, {30, 30}}, true},
            {{{50, 34}, {10, 10}}, false, glm::mat4{mg::transformation(mir_orientation_left)}}}, 0},
        SceneCase{"oriented_above_disjoint", {{{{12, 34}, {30, 30}}, true},
            {.area = {{50, 34}, {10, 10}}, .orientation = mir_orientation_left}}, 0},
        SceneCase{"oriented_footprint_avoids_false_block", {{{{12, 34}, {30, 30}}, true},
            {.area = {{30, 10}, {20, 40}}, .orientation = mir_orientation_left}}, 0},
        SceneCase{"translated_above_overlaps", {{{{12, 34}, {30, 30}}, true},
            {{{50, 34}, {10, 10}}, false,
             glm::translate(glm::mat4{1}, glm::vec3{-45.0f, 0.0f, 0.0f})}}, std::nullopt},
        SceneCase{"tilted_above_blocks", {{{{12, 34}, {30, 30}}, true},
            {{{50, 34}, {10, 10}}, false,
             glm::rotate(glm::mat4{1}, 0.5f, glm::vec3{1.0f, 0.0f, 0.0f})}}, std::nullopt},
        SceneCase{"disjoint_plain_ui", {{{{12, 34}, {30, 30}}, true}, {{{50, 34}, {20, 20}}}}, 0}),
    [](TestParamInfo<SceneCase> const& info) { return std::string{info.param.name}; });

TEST_P(AtomicMixedPresentation, overlay_queues_the_mixed_state_only_when_the_kernel_accepts_it)
{
    auto const& param = GetParam();
    auto const kernel_accepts = param.kernel_accepts;
    auto const expected_destination = geom::Point{} + (param.client_top_left - display_area.top_left);
    auto const earlier_fb = std::make_shared<NiceMock<mt::MockKMSFramebuffer>>();
    if (!kernel_accepts)
    {
        ASSERT_TRUE(sink->overlay({fullscreen_element(earlier_fb)}));
    }
    auto primary_fb = std::make_shared<NiceMock<mt::MockKMSFramebuffer>>();
    auto client_fb = make_client_framebuffer();
    std::weak_ptr<mg::Framebuffer> const weak_primary{primary_fb};
    std::weak_ptr<mg::Framebuffer> const weak_client{client_fb};

    auto capabilities = legal_capabilities();
    capabilities.overlays.front().id = first_overlay_id;
    auto second_overlay = legal_capabilities().overlays.front();
    second_overlay.id = second_overlay_id;
    capabilities.overlays.push_back(std::move(second_overlay));
    ON_CALL(*output, plane_capabilities())
        .WillByDefault(Return(shared_capabilities(std::move(capabilities))));

    if (kernel_accepts)
    {
        EXPECT_CALL(*output, page_flip(_)).Times(0);
    }
    else
    {
        EXPECT_CALL(*output, page_flip(Ref(*earlier_fb))).WillOnce(Return(true));
    }
    EXPECT_CALL(*output, set_crtc(_)).Times(0);
    EXPECT_CALL(
        *output,
        test_presentation(mixed_state_matcher(
            primary_fb.get(), client_fb.get(), first_overlay_id, expected_destination)))
        .WillOnce(Return(kernel_accepts));

    {
        auto const primary = fullscreen_element(primary_fb);
        auto const client = client_element(client_fb, param.client_top_left);
        EXPECT_EQ(kernel_accepts, sink->overlay({primary, client}));
    }

    primary_fb.reset();
    client_fb.reset();

    if (kernel_accepts)
    {
        EXPECT_FALSE(weak_primary.expired());
        EXPECT_FALSE(weak_client.expired());

        auto const replacement_fb = std::make_shared<NiceMock<mt::MockKMSFramebuffer>>();
        EXPECT_TRUE(sink->overlay({fullscreen_element(replacement_fb)}));
        EXPECT_TRUE(weak_primary.expired());
        EXPECT_TRUE(weak_client.expired());
    }
    else
    {
        EXPECT_TRUE(weak_primary.expired());
        EXPECT_TRUE(weak_client.expired());
        EXPECT_TRUE(sink->post());
    }
}

INSTANTIATE_TEST_SUITE_P(
    KernelAcceptance,
    AtomicMixedPresentation,
    Values(
        MixedPresentationCase{"kernel_rejects", false, client_top_left},
        MixedPresentationCase{"kernel_accepts", true, client_top_left},
        MixedPresentationCase{"partly_outside_output", true, client_top_left_outside_output}),
    [](TestParamInfo<MixedPresentationCase> const& info)
    {
        return std::string{info.param.name};
    });

TEST_P(AtomicMixedRejection, overlay_rejects_ineligible_elements)
{
    auto const& param = GetParam();
    ON_CALL(*output, plane_capabilities())
        .WillByDefault(Return(param.capabilities));

    auto const primary = mg::DisplayElement{
        param.primary_destination,
        {{0, 0}, {display_area.size.width.as_int(), display_area.size.height.as_int()}},
        std::make_shared<NiceMock<mt::MockKMSFramebuffer>>()};

    std::shared_ptr<mg::Framebuffer> client_fb;
    if (param.plain_client_framebuffer)
    {
        client_fb = std::make_shared<NiceMock<mt::MockKMSFramebuffer>>();
    }
    else
    {
        client_fb = make_client_framebuffer({param.client_format, param.client_modifier}, param.client_layout);
    }

    mg::DisplayElement const client_element{param.client_destination, param.client_source, client_fb};

    EXPECT_CALL(*output, test_presentation(_)).Times(0);

    EXPECT_FALSE(sink->overlay({primary, client_element}));
}

INSTANTIATE_TEST_SUITE_P(
    ElementEligibility,
    AtomicMixedRejection,
    Values(
        MixedRejectionCase{.name = "no_plane_capabilities", .capabilities = nullptr},
        MixedRejectionCase{.name = "primary_not_fullscreen",
                           .primary_destination = {display_area.top_left, client_size}},
        MixedRejectionCase{.name = "plain_client_framebuffer", .plain_client_framebuffer = true},
        MixedRejectionCase{.name = "client_format_not_nv12", .client_format = DRM_FORMAT_XRGB8888},
        MixedRejectionCase{.name = "client_modifier_not_supported", .client_modifier = other_modifier},
        MixedRejectionCase{.name = "client_layout_not_gl",
                           .client_layout = mg::gl::Texture::Layout::TopRowFirst},
        MixedRejectionCase{.name = "scaled_client_destination",
                           .client_destination = {client_top_left, client_size / 2}},
        MixedRejectionCase{.name = "cropped_client_source",
                           .client_source = {{0, 0}, {client_size.width.as_value() / 2,
                                                      client_size.height.as_value() / 2}}},
        MixedRejectionCase{.name = "offset_client_source",
                           .client_source = {{1, 0}, {client_size.width.as_value(), client_size.height.as_value()}}},
        MixedRejectionCase{.name = "client_outside_output",
                           .client_destination = {client_top_left_clear_of_output, client_size}}),
    [](TestParamInfo<MixedRejectionCase> const& info)
    {
        return std::string{info.param.name};
    });

TEST_F(DmaBufImportTest, gbm_import_failure_returns_no_framebuffer)
{
    EXPECT_CALL(gbm, gbm_bo_import(_, _, _, _))
        .WillOnce(SetErrnoAndReturn(EINVAL, nullptr));
    EXPECT_CALL(drm, drmModeAddFB2WithModifiers(_, _, _, _, _, _, _, _, _, _)).Times(0);

    EXPECT_EQ(allocator.framebuffer_for(make_buffer()), nullptr);
}

TEST_F(DmaBufImportTest, add_fb_failure_releases_the_bo_and_the_buffer)
{
    auto buffer = make_buffer();
    std::weak_ptr<mg::DMABufBuffer> const weak_buffer{buffer};

    EXPECT_CALL(drm, drmModeAddFB2WithModifiers(_, _, _, _, _, _, _, _, _, _))
        .WillOnce(Return(-EINVAL));
    EXPECT_CALL(gbm, gbm_bo_destroy(gbm.fake_gbm.bo)).Times(1);
    EXPECT_CALL(drm, drmModeRmFB(_, _)).Times(0);

    EXPECT_EQ(allocator.framebuffer_for(buffer), nullptr);

    buffer.reset();
    EXPECT_TRUE(weak_buffer.expired());
}

TEST_F(DmaBufImportTest, framebuffer_keeps_the_bo_and_the_buffer_until_destroyed)
{
    auto buffer = make_buffer();
    std::weak_ptr<mg::DMABufBuffer> const weak_buffer{buffer};

    EXPECT_CALL(gbm, gbm_bo_destroy(_)).Times(0);
    EXPECT_CALL(drm, drmModeRmFB(_, _)).Times(0);

    auto framebuffer = allocator.framebuffer_for(buffer);
    ASSERT_NE(framebuffer, nullptr);

    buffer.reset();
    EXPECT_FALSE(weak_buffer.expired());

    Mock::VerifyAndClearExpectations(&gbm);
    Mock::VerifyAndClearExpectations(&drm);
    EXPECT_CALL(gbm, gbm_bo_destroy(gbm.fake_gbm.bo)).Times(1);
    EXPECT_CALL(drm, drmModeRmFB(_, _)).Times(1);

    framebuffer.reset();
    EXPECT_TRUE(weak_buffer.expired());
}

TEST_P(DmaBufImportModifierTest, passes_the_modifier_to_gbm_and_addfb)
{
    auto const& param = GetParam();
    auto const buffer = make_buffer();
    buffer->modifier_ = param.buffer_modifier;

    EXPECT_CALL(
        gbm,
        gbm_bo_import(
            _,
            _,
            ResultOf(
                [](void* data)
                {
                    return static_cast<gbm_import_fd_modifier_data const*>(data)->modifier;
                },
                param.gbm_modifier),
            _))
        .WillOnce(Return(gbm.fake_gbm.bo));
    EXPECT_CALL(drm, drmModeAddFB2WithModifiers(_, _, _, _, _, _, _, Pointee(param.addfb_modifier), _, _))
        .WillOnce(DoAll(SetArgPointee<8>(fb_id), Return(0)));

    EXPECT_NE(allocator.framebuffer_for(buffer), nullptr);
}

INSTANTIATE_TEST_SUITE_P(
    ModifierImport,
    DmaBufImportModifierTest,
    Values(
        ImportModifierCase{
            .name = "linear",
            .buffer_modifier = DRM_FORMAT_MOD_LINEAR,
            .gbm_modifier = DRM_FORMAT_MOD_LINEAR,
            .addfb_modifier = DRM_FORMAT_MOD_LINEAR},
        ImportModifierCase{
            .name = "nonlinear",
            .buffer_modifier = DRM_FORMAT_MOD_SAMSUNG_64_32_TILE,
            .gbm_modifier = DRM_FORMAT_MOD_SAMSUNG_64_32_TILE,
            .addfb_modifier = DRM_FORMAT_MOD_SAMSUNG_64_32_TILE},
        ImportModifierCase{
            .name = "absent",
            .buffer_modifier = std::nullopt,
            .gbm_modifier = DRM_FORMAT_MOD_NONE,
            .addfb_modifier = DRM_FORMAT_MOD_INVALID},
        ImportModifierCase{
            .name = "invalid",
            .buffer_modifier = DRM_FORMAT_MOD_INVALID,
            .gbm_modifier = DRM_FORMAT_MOD_INVALID,
            .addfb_modifier = DRM_FORMAT_MOD_INVALID}),
    [](TestParamInfo<ImportModifierCase> const& info)
    {
        return std::string{info.param.name};
    });
