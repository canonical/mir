/*
 * Copyright © Canonical Ltd.
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
 */

#include "display_sink.h"
#include <mir/graphics/kms_cpu_addressable_display_provider.h>
#include "kms_output.h"
#include "plane_capabilities.h"
#include <mir/graphics/cpu_addressable_fb.h>
#include "gbm_display_allocator.h"
#include <mir/errno_utils.h>
#include <mir/fd.h>
#include <mir/graphics/display_report.h>
#include <mir/graphics/drm_formats.h>
#include <mir/graphics/platform.h>

#include <mir/log.h>
#include <mir/graphics/dmabuf_buffer.h>

#include <algorithm>
#include <array>
#include <boost/throw_exception.hpp>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <drm_fourcc.h>

#include <drm_mode.h>
#include <gbm.h>
#include <memory>
#include <optional>
#include <ranges>
#include <span>
#include <stdexcept>
#include <utility>
#include <chrono>
#include <xf86drm.h>
#include <xf86drmMode.h>

namespace mg = mir::graphics;
namespace mga = mir::graphics::atomic;
namespace geom = mir::geometry;
namespace mgk = mir::graphics::kms;

mga::DisplaySink::DisplaySink(
    mir::Fd drm_fd,
    std::shared_ptr<struct gbm_device> gbm,
    mga::BypassOption,
    mga::PlanePromotion plane_promotion,
    std::shared_ptr<DisplayReport> const& listener,
    std::shared_ptr<KMSOutput> output,
    geom::Rectangle const& area,
    glm::mat2 const& transformation,
    std::shared_ptr<GbmQuirks> const& gbm_quirks)
    : DmaBufDisplayAllocator(gbm, drm_fd),
      gbm{std::move(gbm)},
      listener(listener),
      output{std::move(output)},
      plane_promotion{plane_promotion},
      area(area),
      transform{transformation},
      needs_set_crtc{false},
      gbm_quirks{gbm_quirks}
{
    listener->report_successful_setup_of_native_resources();

    if (this->output->has_crtc_mismatch())
    {
        mir::log_info("Clearing screen due to differing encountered and target modes");
        // TODO: Pull a supported format out of KMS rather than assuming XRGB8888
        auto initial_fb = std::make_shared<graphics::CPUAddressableFB>(
            std::move(drm_fd),
            false,
            DRMFormat{DRM_FORMAT_XRGB8888},
            area.size);

        auto mapping = initial_fb->map_writeable();
        ::memset(mapping->data(), 24, mapping->len());

        visible_state = PresentationState::full_scene(std::move(initial_fb));
        this->output->set_crtc(visible_state->primary());
        listener->report_successful_drm_mode_set_crtc_on_construction();
    }
    listener->report_successful_display_construction();
}

mga::DisplaySink::~DisplaySink() = default;

geom::Rectangle mga::DisplaySink::view_area() const
{
    return area;
}

glm::mat2 mga::DisplaySink::transformation() const
{
    return transform;
}

void mga::DisplaySink::set_transformation(glm::mat2 const& t, geometry::Rectangle const& a)
{
    transform = t;
    area = a;
}

bool mga::DisplaySink::overlay(std::vector<DisplayElement> const& renderable_list)
{
    if (renderable_list.size() == 1)
    {
        if (renderable_list[0].screen_positon != view_area())
        {
            return false;
        }

        if (renderable_list[0].source_position.top_left != geom::PointF{0, 0} ||
            renderable_list[0].source_position.size.width.as_value() != view_area().size.width.as_int() ||
            renderable_list[0].source_position.size.height.as_value() != view_area().size.height.as_int())
        {
            return false;
        }

        if (auto fb = std::dynamic_pointer_cast<graphics::FBHandle>(renderable_list[0].buffer))
        {
            next_state = PresentationState::full_scene(std::move(fb));
            return true;
        }
        return false;
    }
    else
    {
        // TODO map display elements to atomic KMS planes
    }

    return false;
}

namespace
{
auto const identity_transform = glm::mat4{1};
constexpr mga::PropertyValue opaque_alpha{0xffff};

auto is_directly_scannable(mg::Renderable const& renderable) -> bool
{
    return renderable.alpha() == 1.0f &&
        renderable.transformation() == identity_transform &&
        renderable.orientation() == mir_orientation_normal &&
        renderable.mirror_mode() == mir_mirror_mode_none &&
        !renderable.clip_area();
}

auto has_legal_overlay_plane(mga::CrtcPlaneCapabilities const& capabilities, uint64_t modifier) -> bool
{
    auto const format_modifier = mga::FormatModifier{DRM_FORMAT_NV12, modifier};
    return std::ranges::any_of(capabilities.overlays, [&](mga::PlaneCapability const& plane)
    {
        return plane.can_be_above_primary &&
            plane.bt709_limited.has_value() &&
            std::ranges::find(plane.formats, format_modifier) != plane.formats.end() &&
            (!plane.alpha_max || plane.alpha_max == opaque_alpha);
    });
}

auto is_overlay_eligible(mg::Renderable const& renderable, mga::CrtcPlaneCapabilities const& capabilities) -> bool
{
    if (!is_directly_scannable(renderable))
        return false;

    auto const buffer = renderable.buffer();
    if (!buffer)
        return false;

    auto const dmabuf = dynamic_cast<mg::DMABufBuffer*>(buffer->native_buffer_base());
    if (!dmabuf ||
        dmabuf->format() != DRM_FORMAT_NV12 ||
        dmabuf->layout() != mg::gl::Texture::Layout::GL)
    {
        return false;
    }

    auto const modifier = dmabuf->modifier();
    return modifier && *modifier != DRM_FORMAT_MOD_INVALID &&
        has_legal_overlay_plane(capabilities, *modifier) &&
        renderable.src_bounds() == geom::RectangleD{{0, 0}, dmabuf->size()} &&
        renderable.screen_position().size == dmabuf->size();
}

auto pixel_area(geom::Rectangle const& rect) -> int
{
    return rect.size.width.as_int() * rect.size.height.as_int();
}

auto is_2d_affine(glm::mat4 const& transform) -> bool
{
    // screen_to_gl_coords sets w = 1 - z, so a transform only preserves the footprint
    // when it leaves the z = 0 plane fixed and keeps w at 1. glm is column-major:
    // transform[column][row].
    return transform[0][3] == 0 && transform[1][3] == 0 && transform[3][3] == 1 &&
        transform[0][2] == 0 && transform[1][2] == 0 && transform[3][2] == 0;
}

auto oriented_footprint(geom::Rectangle const& rect, MirOrientation orientation) -> geom::Rectangle
{
    // Left and right orientations rotate the rect about its centre and re-anchor it at the
    // same top_left, which swaps its dimensions.
    if (orientation == mir_orientation_left || orientation == mir_orientation_right)
    {
        return {
            rect.top_left,
            geom::Size{geom::Width{rect.size.height.as_int()}, geom::Height{rect.size.width.as_int()}}};
    }
    return rect;
}

// Mirror the shader, which composes the transform about the centre:
// (transform * (position - centre)) + centre.
auto transformed_bounds(glm::mat4 const& transform, geom::Rectangle const& rect) -> geom::Rectangle
{
    auto const rect_f = geom::RectangleF{geom::PointF{rect.top_left}, geom::SizeF{rect.size}};
    auto const [centre_x, centre_y] = rect_f.centre();
    auto const [half_width, half_height] = rect_f.size / 2.0f;

    auto const corners = std::array{
        glm::vec4{-half_width.as_value(), -half_height.as_value(), 0.0f, 1.0f},
        glm::vec4{-half_width.as_value(),  half_height.as_value(), 0.0f, 1.0f},
        glm::vec4{ half_width.as_value(), -half_height.as_value(), 0.0f, 1.0f},
        glm::vec4{ half_width.as_value(),  half_height.as_value(), 0.0f, 1.0f}};

    auto transformed = std::array<glm::vec4, 4>{};
    std::ranges::transform(corners, transformed.begin(), [&](auto const& c) { return transform * c; });
    auto const [min_x, max_x] = std::ranges::minmax(transformed, {}, [](auto const& v) { return v.x; });
    auto const [min_y, max_y] = std::ranges::minmax(transformed, {}, [](auto const& v) { return v.y; });

    // Round outward so float noise can never shrink the bound.
    auto const left   = std::floor(centre_x.as_value() + min_x.x);
    auto const top    = std::floor(centre_y.as_value() + min_y.y);
    auto const right  = std::ceil (centre_x.as_value() + max_x.x);
    auto const bottom = std::ceil (centre_y.as_value() + max_y.y);

    return geom::Rectangle{
        {geom::X{static_cast<int>(left)}, geom::Y{static_cast<int>(top)}},
        {geom::Width{static_cast<int>(right - left)}, geom::Height{static_cast<int>(bottom - top)}}};
}

auto footprint_bounds(mg::Renderable const& renderable) -> std::optional<geom::Rectangle>
{
    auto const transform = renderable.transformation();

    if (transform == identity_transform)
        return oriented_footprint(renderable.screen_position(), renderable.orientation());

    if (!is_2d_affine(transform))
        return std::nullopt;

    auto const oriented = oriented_footprint(renderable.screen_position(), renderable.orientation());
    return transformed_bounds(transform, oriented);
}

auto content_above_blocks(
    std::span<std::optional<geom::Rectangle> const> content_above,
    geom::Rectangle const& candidate_bounds) -> bool
{
    return std::ranges::any_of(content_above, [&](auto const& bounds) { return !bounds || bounds->overlaps(candidate_bounds); });
}

auto select_direct_candidate(
    std::vector<std::shared_ptr<mg::Renderable>> const& renderables,
    geom::Rectangle const& sink_area,
    mga::CrtcPlaneCapabilities const& capabilities) -> std::optional<std::size_t>
{
    std::optional<std::size_t> best;
    int best_area{0};

    auto const transformed_bounds =
        renderables |
        std::ranges::views::transform([](auto const& renderable) { return footprint_bounds(*renderable); })
        | std::ranges::to<std::vector>();

    for (auto candidate_index = std::size_t{0}; candidate_index < renderables.size(); ++candidate_index)
    {
        auto const& candidate_bounds = transformed_bounds[candidate_index];
        if (!candidate_bounds || candidate_bounds->size.width.as_int() <= 0 ||
            candidate_bounds->size.height.as_int() <= 0 || !sink_area.contains(*candidate_bounds))
            continue;

        auto const& renderable = renderables[candidate_index];
        if (!is_overlay_eligible(*renderable, capabilities))
            continue;

        auto const content_above = std::span{transformed_bounds}.subspan(candidate_index + 1);
        if (content_above_blocks(content_above, *candidate_bounds))
            continue;

        auto const candidate_area = pixel_area(*candidate_bounds);
        // >= keeps the frontmost of equal-area candidates, as index increases towards the front
        if (!best || candidate_area >= best_area)
        {
            best = candidate_index;
            best_area = candidate_area;
        }
    }

    return best;
}
}

auto mga::DisplaySink::plan_presentation(std::vector<std::shared_ptr<Renderable>> const& renderables)
    -> std::optional<LogicalStacking>
{
    auto const plane_capabilities = output->plane_capabilities();
    // Overlay destinations use CRTC pixels: the logical output rectangle must map 1:1 onto the mode.
    auto const unscaled = area.size == output->size();
    if (plane_promotion != PlanePromotion::available ||
        !plane_capabilities ||
        !unscaled ||
        transform != glm::mat2{1})
    {
        return std::nullopt;
    }

    auto const candidate = select_direct_candidate(renderables, area, *plane_capabilities);
    if (!candidate)
        return std::nullopt;

    return LogicalStacking{{
        CompositingLayer{
            renderables |
                std::views::filter([&](auto const& renderable) { return renderable != renderables[*candidate]; }) |
                std::ranges::to<std::vector>(),
        },
        DirectLayer{renderables[*candidate]},
    }};
}

void mga::DisplaySink::for_each_display_sink(std::function<void(graphics::DisplaySink&)> const& f)
{
    f(*this);
}

bool mga::DisplaySink::set_crtc(FBHandle const& forced_frame)
{
    /*
     * Note that failure to set the CRTC is not a fatal error. This can
     * happen under normal conditions when resizing VirtualBox (which
     * actually removes and replaces the virtual output each time so
     * sometimes it's really not there). Xorg often reports similar
     * errors, and it's not fatal.
     */
    if (!output->set_crtc(forced_frame))
    {
        mir::log_error("Failed to set DRM CRTC. "
            "Screen contents may be incomplete. "
            "Try plugging the monitor in again.");
        return false;
    }
    return true;
}

bool mga::DisplaySink::post()
{
    if (!next_state)
    {
        // Hey! No one has given us a next frame yet, so we don't have to change what's onscreen.
        // Sweet! We can just bail.
        return true;
    }
    /*
     * Otherwise, pull the next frame into the pending slot
     */
    scheduled_state = std::move(next_state);
    next_state.reset();

    /*
     * Try to schedule a page flip as first preference to avoid tearing.
     * We wait synchronously for this to complete.
     */
    if (!needs_set_crtc && !output->page_flip(scheduled_state->primary()))
        needs_set_crtc = true;

    /*
     * Fallback blitting: Not pretty, since it may tear. VirtualBox seems
     * to need to do this on every frame. [will complete in this thread]
     */
    auto presentation_succeeded = true;
    if (needs_set_crtc)
    {
        presentation_succeeded = set_crtc(scheduled_state->primary());
        // SetCrtc is immediate, so the FB is now visible and we have nothing pending

        needs_set_crtc = false;
    }

    if (presentation_succeeded)
        visible_state = std::move(scheduled_state);
    scheduled_state.reset();

    using namespace std::chrono_literals;  // For operator""ms()

    // Predicted worst case render time for the next frame...
    auto predicted_render_time = 50ms;

    /*
     * TODO: Make a sensible predicited_render_time
     */

    recommend_sleep = 0ms;
    auto const min_frame_interval = 1000ms / output->max_refresh_rate();
    if (predicted_render_time < min_frame_interval)
        recommend_sleep = min_frame_interval - predicted_render_time;

    return presentation_succeeded;
}

std::chrono::milliseconds mga::DisplaySink::recommended_sleep() const
{
    return recommend_sleep;
}

void mga::DisplaySink::schedule_set_crtc()
{
    output->reset();
    needs_set_crtc = true;
}

auto mga::DisplaySink::drm_fd() const -> mir::Fd
{
    return mir::Fd{mir::IntOwnedFd{output->drm_fd()}};
}

auto mga::DisplaySink::gbm_device() const -> std::shared_ptr<struct gbm_device>
{
    return gbm;
}

void mga::DisplaySink::set_next_image(FramebufferPtr content)
{
    std::vector<DisplayElement> const single_buffer = {
        DisplayElement {
            view_area(),
            geom::RectangleF{
                {0, 0},
                {view_area().size.width.as_value(), view_area().size.height.as_value()}},
            std::move(content)
        }
    };
    if (!overlay(single_buffer))
    {
        // Oh, oh! We should be *guaranteed* to “overlay” a single Framebuffer; this is likely a programming error
        BOOST_THROW_EXCEPTION((std::runtime_error{"Failed to post buffer to display"}));
    }
}

namespace {
auto const MAX_PLANES = 4zu;

auto get_import_buffers(std::shared_ptr<mg::DMABufBuffer> buffer)
    -> std::tuple<std::array<int, 4>, std::array<int, 4>, std::array<int, 4>, std::array<uint64_t, 4>>
{
    auto const plane_descriptors = buffer->planes();
    assert(plane_descriptors.size() <= MAX_PLANES);

    // std::array becuase we can't really return int[4]
    std::array<int, 4> dmabuf_fds = {0};
    std::array<int, 4> pitches = {0};
    std::array<int, 4> offsets = {0};
    std::array<uint64_t, 4> modifiers = {0};

    for (std::size_t i = 0; i < std::min(MAX_PLANES, plane_descriptors.size()); i++)
    {
        dmabuf_fds[i] = plane_descriptors[i].dma_buf;
        pitches[i] = plane_descriptors[i].stride;
        offsets[i] = plane_descriptors[i].offset;
        modifiers[i] = buffer->modifier().value_or(DRM_FORMAT_MOD_INVALID);
    }

    return std::make_tuple(dmabuf_fds, pitches, offsets, modifiers);
}

auto import_gbm_bo(
    std::shared_ptr<struct gbm_device> gbm,
    std::shared_ptr<mg::DMABufBuffer> buffer,
    std::array<int, 4> dmabuf_fds,
    std::array<int, 4> pitches,
    std::array<int, 4> offsets) -> std::shared_ptr<struct gbm_bo>
{
    auto const plane_descriptors = buffer->planes();

    gbm_import_fd_modifier_data import_data = {
        .width = buffer->size().width.as_uint32_t(),
        .height = buffer->size().height.as_uint32_t(),
        .format = buffer->format(),
        .num_fds = static_cast<uint32_t>(std::min(plane_descriptors.size(), MAX_PLANES)),
        .fds = {dmabuf_fds[0], dmabuf_fds[1], dmabuf_fds[2], dmabuf_fds[3]},
        .strides = {pitches[0], pitches[1], pitches[2], pitches[3]},
        .offsets = {offsets[0], offsets[1], offsets[2], offsets[3]},
        .modifier = buffer->modifier().value_or(DRM_FORMAT_MOD_NONE),
    };

    auto* gbm_bo = gbm_bo_import(gbm.get(), GBM_BO_IMPORT_FD_MODIFIER, (void*)&import_data, GBM_BO_USE_SCANOUT);
    if (!gbm_bo)
    {
        mir::log_debug(
            "Failed to import buffer type %s:%s (%s [%i])",
            buffer->format().name(),
            mg::drm_modifier_to_string(buffer->modifier().value_or(DRM_FORMAT_MOD_INVALID)).c_str(),
            mir::errno_to_cstr(errno),
            errno);
        return {};
    }
    return std::shared_ptr<struct gbm_bo>(gbm_bo, &gbm_bo_destroy);
}

auto drm_fb_id_from_dma_buffer(
    mir::Fd drm_fd, std::shared_ptr<struct gbm_device> const gbm, std::shared_ptr<mg::DMABufBuffer> buffer)
    -> std::shared_ptr<uint32_t>
{
    auto [dmabuf_fds, pitches, offsets, modifiers] = get_import_buffers(buffer);
    auto gbm_bo = import_gbm_bo(gbm, buffer, dmabuf_fds, pitches, offsets);
    if (!gbm_bo)
    {
        return {};
    }

    auto const plane_descriptors = buffer->planes();
    uint32_t gem_handles[MAX_PLANES] = {0};
    for (std::size_t i = 0; i < std::min(MAX_PLANES, plane_descriptors.size()); i++)
        gem_handles[i] = gbm_bo_get_handle_for_plane(gbm_bo.get(), i).u32;

    // Note that we capture `gbm_bo` so that its lifetime matches that of the fb_id
    auto fb_id = std::shared_ptr<uint32_t>{
        new uint32_t{0},
        [drm_fd, gbm_bo, buffer](uint32_t* fb_id)
        {
            if (*fb_id)
                drmModeRmFB(drm_fd, *fb_id);

            delete fb_id;
        }};

    auto [width, height] = buffer->size();
    int ret = drmModeAddFB2WithModifiers(
        drm_fd,
        width.as_uint32_t(),
        height.as_uint32_t(),
        buffer->format(),
        gem_handles,
        reinterpret_cast<uint32_t*>(pitches.data()),
        reinterpret_cast<uint32_t*>(offsets.data()),
        modifiers.data(),
        fb_id.get(),
        DRM_MODE_FB_MODIFIERS);

    if (ret)
    {
        mir::log_warning("drmModeAddFB2WithModifiers returned an error: %d", ret);
        return {};
    }

    return fb_id;
}
}

auto mga::DmaBufDisplayAllocator::framebuffer_for(std::shared_ptr<DMABufBuffer> buffer) -> std::unique_ptr<Framebuffer>
{
    auto fb_id = drm_fb_id_from_dma_buffer(drm_fd(), gbm, buffer);

    if (!fb_id)
    {
        return {};
    }

    struct AtomicKmsFbHandle : public mg::FBHandle
    {
        AtomicKmsFbHandle(std::shared_ptr<uint32_t> fb_handle, geometry::Size size) :
            kms_fb_id{fb_handle},
            size_{size}
        {
        }

        virtual auto size() const -> geometry::Size override
        {
            return size_;
        }

        virtual operator uint32_t() const override
        {
            return *kms_fb_id;
        }

    private:
        std::shared_ptr<uint32_t> kms_fb_id;
        geometry::Size size_;
    };

    buffer->on_consumed();

    return std::make_unique<AtomicKmsFbHandle>(fb_id, buffer->size());
}

auto mga::DisplaySink::maybe_create_allocator(DisplayAllocator::Tag const& type_tag)
    -> DisplayAllocator*
{
    if (dynamic_cast<mg::CPUAddressableDisplayAllocator::Tag const*>(&type_tag))
    {
        if (!kms_allocator)
        {
            kms_allocator = kms::CPUAddressableDisplayAllocator::create_if_supported(drm_fd(), output->size());
        }
        return kms_allocator.get();
    }
    if (dynamic_cast<mg::GBMDisplayAllocator::Tag const*>(&type_tag))
    {
        if (!gbm_allocator)
        {
            gbm_allocator = std::make_unique<GBMDisplayAllocator>(drm_fd(), gbm, output->size(), gbm_quirks);
        }
        return gbm_allocator.get();
    }
    if(dynamic_cast<DmaBufDisplayAllocator::Tag const*>(&type_tag))
    {
        if (!bypass_allocator)
        {
           bypass_allocator = std::make_shared<DmaBufDisplayAllocator>(gbm, drm_fd());
        }
        return bypass_allocator.get();
    }
    return nullptr;
}
