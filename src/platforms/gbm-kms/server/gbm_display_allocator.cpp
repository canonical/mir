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

#include "gbm_display_allocator.h"
#include "kms_framebuffer.h"

#include <drm_fourcc.h>
#include <xf86drmMode.h>
#include <gbm.h>

namespace mg = mir::graphics;
namespace mgg = mir::graphics::gbm;
namespace geom = mir::geometry;

mgg::GBMDisplayAllocator::GBMDisplayAllocator(mir::Fd drm_fd, std::shared_ptr<struct gbm_device> gbm, geom::Size size)
    : fd{std::move(drm_fd)},
      gbm{std::move(gbm)},
      size{size}
{
}

auto mgg::GBMDisplayAllocator::supported_formats() const -> std::vector<DRMFormat>
{
    // TODO: Pull out of KMS plane info
    return { DRMFormat{DRM_FORMAT_XRGB8888}, DRMFormat{DRM_FORMAT_ARGB8888}};
}

auto mgg::GBMDisplayAllocator::modifiers_for_format(DRMFormat /*format*/) const -> std::vector<uint64_t>
{
    // TODO: Pull out off KMS plane info
    return {};
}

namespace
{
using LockedFrontBuffer = std::unique_ptr<gbm_bo, std::function<void(gbm_bo*)>>;

class GBMBoFramebuffer : public mg::FBHandle
{
public:
    operator uint32_t() const override
    {
        return *fb_id;
    }

    auto size() const -> geom::Size override
    {
        return
            geom::Size{
                gbm_bo_get_width(bo.get()),
                gbm_bo_get_height(bo.get())};
    }

    GBMBoFramebuffer(LockedFrontBuffer bo, std::shared_ptr<uint32_t const> fb)
        : bo{std::move(bo)},
          fb_id{std::move(fb)}
    {
    }
private:
    LockedFrontBuffer const bo;
    std::shared_ptr<uint32_t const> const fb_id;
};

namespace
{
auto create_gbm_surface(gbm_device* gbm, geom::Size size, mg::DRMFormat format, std::span<uint64_t> modifiers)
    -> std::shared_ptr<gbm_surface>
{
    auto const surface =
        [&]()
        {
            if (modifiers.empty())
            {
                // If we have no no modifiers don't use the with-modifiers creation path.
                auto foo = GBM_BO_USE_RENDERING | GBM_BO_USE_SCANOUT;
                return gbm_surface_create(
                    gbm,
                    size.width.as_uint32_t(), size.height.as_uint32_t(),
                    format,
                    foo);
            }
            else
            {
                return gbm_surface_create_with_modifiers2(
                    gbm,
                    size.width.as_uint32_t(), size.height.as_uint32_t(),
                    format,
                    modifiers.data(),
                    modifiers.size(),
                    GBM_BO_USE_RENDERING | GBM_BO_USE_SCANOUT);
            }
        }();

    if (!surface)
    {
        BOOST_THROW_EXCEPTION((
            std::system_error{
                errno,
                std::system_category(),
                "Failed to create GBM surface"}));

    }
    return std::shared_ptr<gbm_surface>{
        surface,
        [](auto surface) { gbm_surface_destroy(surface); }};
}
}

class GBMSurfaceImpl : public mgg::GBMDisplayAllocator::GBMSurface
{
public:
    GBMSurfaceImpl(mir::Fd drm_fd, gbm_device* gbm, geom::Size size, mg::DRMFormat const format, std::span<uint64_t> modifiers)
        : drm_fd{std::move(drm_fd)},
          surface{create_gbm_surface(gbm, size, format, modifiers)}
    {
    }

    GBMSurfaceImpl(GBMSurfaceImpl const&) = delete;
    auto operator=(GBMSurfaceImpl const&) -> GBMSurfaceImpl const& = delete;

    operator gbm_surface*() const override
    {
        return surface.get();
    }

    auto claim_buffer() -> std::unique_ptr<mg::Buffer> override
    {
        if (!gbm_surface_has_free_buffers(surface.get()))
        {
            BOOST_THROW_EXCEPTION((
                std::system_error{
                    EBUSY,
                    std::system_category(),
                    "Too many buffers consumed from GBM surface"}));
        }

        LockedFrontBuffer bo{
            gbm_surface_lock_front_buffer(surface.get()),
            [shared_surface = surface](gbm_bo* bo) { gbm_surface_release_buffer(shared_surface.get(), bo); }};

        if (!bo)
        {
            BOOST_THROW_EXCEPTION((std::runtime_error{"Failed to acquire GBM front buffer"}));
        }

        return std::make_unique<mgg::GBMBuffer>(drm_fd, std::move(bo));
    }
private:
    mir::Fd const drm_fd;
    std::shared_ptr<gbm_surface> const surface;
};
}

auto mgg::GBMDisplayAllocator::make_surface(DRMFormat format, std::span<uint64_t> modifiers)
    -> std::unique_ptr<GBMSurface>
{
    return std::make_unique<GBMSurfaceImpl>(fd, gbm.get(), size, format, modifiers);
}

mir::graphics::gbm::GBMBuffer::GBMBuffer(mir::Fd drm_fd, LockedFrontBuffer front_buffer) :
    front_buffer{std::move(front_buffer)}, 
    drm_fd{drm_fd}
{
}

mir::graphics::BufferID mir::graphics::gbm::GBMBuffer::id() const
{
    // https://stackoverflow.com/a/21574751
    return BufferID{static_cast<uint32_t>(reinterpret_cast<uintptr_t>(front_buffer.get()))};
}

mir::geometry::Size mir::graphics::gbm::GBMBuffer::size() const
{
    return geometry::Size{gbm_bo_get_width(front_buffer.get()), gbm_bo_get_height(front_buffer.get())};
}

MirPixelFormat mir::graphics::gbm::GBMBuffer::pixel_format() const
{
    auto const mapping = std::unordered_map<uint32_t, MirPixelFormat>{
        {GBM_FORMAT_ABGR8888, mir_pixel_format_abgr_8888},
        {GBM_FORMAT_XBGR8888, mir_pixel_format_xbgr_8888},
        {GBM_FORMAT_ARGB8888, mir_pixel_format_argb_8888},
        {GBM_FORMAT_XRGB8888, mir_pixel_format_xrgb_8888},
        {GBM_FORMAT_BGR888, mir_pixel_format_bgr_888},
        {GBM_FORMAT_RGB888, mir_pixel_format_rgb_888},
        {GBM_FORMAT_RGB565, mir_pixel_format_rgb_565},
        {GBM_FORMAT_RGBA5551, mir_pixel_format_rgba_5551},
        {GBM_FORMAT_RGBA4444, mir_pixel_format_rgba_4444},
    };

    auto const gbm_format = gbm_bo_get_format(front_buffer.get());
    if (auto iter = mapping.find(gbm_format); iter != mapping.end())
        return iter->second;

    return mir_pixel_format_invalid;
}

mir::graphics::NativeBufferBase* mir::graphics::gbm::GBMBuffer::native_buffer_base()
{
    return this;
}

auto mgg::GBMBuffer::to_framebuffer() -> std::unique_ptr<Framebuffer>
{
    if (auto cached_fb = static_cast<std::shared_ptr<uint32_t const>*>(gbm_bo_get_user_data(front_buffer.get())))
    {
        return std::make_unique<GBMBoFramebuffer>(std::move(front_buffer), *cached_fb);
    }

    auto fb_id = std::shared_ptr<uint32_t>{
        new uint32_t{0},
        [this](uint32_t* fb_id)
        {
            if (*fb_id)
            {
                drmModeRmFB(drm_fd, *fb_id);
            }
            delete fb_id;
        }};
    uint32_t handles[4] = {gbm_bo_get_handle(front_buffer.get()).u32, 0, 0, 0};
    uint32_t strides[4] = {gbm_bo_get_stride(front_buffer.get()), 0, 0, 0};
    uint32_t offsets[4] = {gbm_bo_get_offset(front_buffer.get(), 0), 0, 0, 0};

    auto format = gbm_bo_get_format(front_buffer.get());

    auto const width = gbm_bo_get_width(front_buffer.get());
    auto const height = gbm_bo_get_height(front_buffer.get());

    /* Create a KMS FB object with the gbm_bo attached to it. */
    auto ret = drmModeAddFB2(drm_fd, width, height, format, handles, strides, offsets, fb_id.get(), 0);
    if (ret)
        return nullptr;

    // It is weird allocating a smart pointer on the heap, but we delete it
    // via gbm_bo_set_user_data()'s destroy_user_data parameter.
    gbm_bo_set_user_data(
        front_buffer.get(),
        new std::shared_ptr<uint32_t>(fb_id),
        [](gbm_bo*, void* fb_ptr) { delete static_cast<std::shared_ptr<uint32_t const>*>(fb_ptr); });

    return std::make_unique<GBMBoFramebuffer>(std::move(front_buffer), std::move(fb_id));
}
