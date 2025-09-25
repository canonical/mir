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

#include "cpu_addressable_fb.h"

#include "mir/log.h"
#include "mir_toolkit/common.h"

#include <sys/mman.h>
#include <xf86drmMode.h>
#include <xf86drm.h>
#include <drm_fourcc.h>

namespace mg = mir::graphics;

class mg::CPUAddressableFB::Buffer : public mir::renderer::software::RWMappableBuffer
{
    template<typename T>
    class Mapping : public mir::renderer::software::Mapping<T>
    {
    public:
        Mapping(
            uint32_t width, uint32_t height,
            uint32_t pitch,
            MirPixelFormat format,
            T* data,
            size_t len)
            : size_{width, height},
              stride_{pitch},
              format_{format},
              data_{data},
              len_{len}
        {
        }

        ~Mapping()
        {
            if (::munmap(const_cast<typename std::remove_const<T>::type *>(data_), len_) == -1)
            {
                // It's unclear how this could happen, but tell *someone* about it if it does!
                log_error("Failed to unmap CPU buffer: %s (%i)", strerror(errno), errno);
            }
        }

        [[nodiscard]]
        auto format() const -> MirPixelFormat
        {
            return format_;
        }

        [[nodiscard]]
        auto stride() const -> mir::geometry::Stride
        {
            return stride_;
        }

        [[nodiscard]]
        auto size() const -> mir::geometry::Size
        {
            return size_;
        }

        [[nodiscard]]
        auto data() -> T*
        {
            return data_;
        }

        [[nodiscard]]
        auto len() const -> size_t
        {
            return len_;
        }

    private:
        mir::geometry::Size const size_;
        mir::geometry::Stride const stride_;
        MirPixelFormat const format_;
        T* const data_;
        size_t const len_;
    };

public:
    ~Buffer()
    {
        struct drm_mode_destroy_dumb params = { gem_handle };

        if (auto const err = drmIoctl(drm_fd, DRM_IOCTL_MODE_DESTROY_DUMB, &params))
        {
            log_error("Failed destroy CPU-accessible buffer: %s (%i)", strerror(-err), -err);
        }
    }

    static auto create_kms_dumb_buffer(mir::Fd drm_fd, DRMFormat format, mir::geometry::Size const& size) ->
        std::unique_ptr<Buffer>
    {
        struct drm_mode_create_dumb params = {};

        params.bpp = 32;
        params.width = size.width.as_uint32_t();
        params.height = size.height.as_uint32_t();

        if (auto const err = drmIoctl(drm_fd, DRM_IOCTL_MODE_CREATE_DUMB, &params))
        {
            BOOST_THROW_EXCEPTION((
                std::system_error{
                    -err,
                    std::system_category(),
                    "Failed to allocate CPU-accessible buffer"}));
        }

        return std::unique_ptr<Buffer>{
            new Buffer{std::move(drm_fd), params, format}};
    }

    auto map_writeable() -> std::unique_ptr<mir::renderer::software::Mapping<std::byte>> override
    {
        auto const data = mmap_buffer(PROT_WRITE);
        return std::make_unique<Mapping<std::byte>>(
            width(), height(),
            pitch(),
            format(),
            static_cast<std::byte*>(data),
            size_);
    }
    auto map_readable() -> std::unique_ptr<mir::renderer::software::Mapping<std::byte const>> override
    {
        auto const data = mmap_buffer(PROT_READ);
        return std::make_unique<Mapping<std::byte const>>(
            width(), height(), pitch(), format(),
            static_cast<std::byte const*>(data),
            size_);
    }

    auto map_rw() -> std::unique_ptr<mir::renderer::software::Mapping<std::byte>> override
    {
        auto const data = mmap_buffer(PROT_READ | PROT_WRITE);
        return std::make_unique<Mapping<std::byte>>(
            width(), height(),
            pitch(),
            format(),
            static_cast<std::byte*>(data),
            size_);
    }

    [[nodiscard]]
    auto handle() const -> uint32_t
    {
        return gem_handle;
    }
    [[nodiscard]]
    auto pitch() const -> uint32_t
    {
        return pitch_;
    }
    [[nodiscard]]
    auto width() const -> uint32_t
    {
        return width_;
    }
    [[nodiscard]]
    auto height() const -> uint32_t
    {
        return height_;
    }

    auto format() const -> MirPixelFormat override
    {
        return format_.as_mir_format().value_or(mir_pixel_format_invalid);
    }

    auto stride() const -> geometry::Stride override
    {
        return geometry::Stride{pitch_};
    }

    auto size() const -> geometry::Size override
    {
        return geometry::Size{width_, height_};
    }
private:
    Buffer(
        mir::Fd fd,
        struct drm_mode_create_dumb const& params,
        DRMFormat format)
        : drm_fd{std::move(fd)},
          width_{params.width},
          height_{params.height},
          pitch_{params.pitch},
          format_{format},
          gem_handle{params.handle},
          size_{static_cast<size_t>(params.size)}    /* params.size is a u64, but the kernel cannot possibly return a value outside the range of size_t
                                                      * On 64bit systems this is trivial, as sizeof(size_t) == sizeof(u64)
                                                      * On 32bit systems sizeof(size_t) < sizeof(u64), so this cast is necessary.
                                                      */
    {
    }

    auto mmap_buffer(int access_mode) -> void*
    {
        struct drm_mode_map_dumb map_request = {};

        map_request.handle = gem_handle;

        if (auto err = drmIoctl(drm_fd, DRM_IOCTL_MODE_MAP_DUMB, &map_request))
        {
            BOOST_THROW_EXCEPTION((
                std::system_error{
                    -err,
                    std::system_category(),
                    "Failed to map buffer for CPU-access"}));
        }

        auto map = mmap(0, size_, access_mode, MAP_SHARED, drm_fd, map_request.offset);
        if (map == MAP_FAILED)
        {
            BOOST_THROW_EXCEPTION((
                std::system_error{
                    errno,
                    std::system_category(),
                    "Failed to mmap() buffer"}));
        }

        return map;
    }

    mir::Fd const drm_fd;
    uint32_t const width_;
    uint32_t const height_;
    uint32_t const pitch_;
    DRMFormat const format_;
    uint32_t const gem_handle;
    size_t const size_;
};

mg::CPUAddressableFB::CPUAddressableFB(
    mir::Fd const& drm_fd,
    bool supports_modifiers,
    DRMFormat format,
    mir::geometry::Size const& size)
    : CPUAddressableFB(drm_fd, supports_modifiers, format, Buffer::create_kms_dumb_buffer(drm_fd, format, size))
{
}

mg::CPUAddressableFB::~CPUAddressableFB()
{
    drmModeRmFB(drm_fd, fb_id);
}

mg::CPUAddressableFB::CPUAddressableFB(
    mir::Fd drm_fd,
    bool supports_modifiers,
    DRMFormat format,
    std::unique_ptr<Buffer> buffer)
    : drm_fd{std::move(drm_fd)},
      fb_id{fb_id_for_buffer(this->drm_fd, supports_modifiers, format, *buffer)},
      buffer{std::move(buffer)}
{
}

auto mg::CPUAddressableFB::map_writeable() -> std::unique_ptr<mir::renderer::software::Mapping<std::byte>>
{
    return buffer->map_writeable();
}

auto mg::CPUAddressableFB::format() const -> MirPixelFormat
{
    return buffer->format();
}

auto mg::CPUAddressableFB::stride() const -> geometry::Stride
{
    return buffer->stride();
}

auto mg::CPUAddressableFB::size() const -> geometry::Size
{
    return buffer->size();
}

mg::CPUAddressableFB::operator uint32_t() const
{
    return fb_id;
}

auto mg::CPUAddressableFB::fb_id_for_buffer(
    mir::Fd const &drm_fd,
    bool supports_modifiers,
    DRMFormat format,
    Buffer const& buf) -> uint32_t
{
    uint32_t fb_id;
    uint32_t const pitches[4] = { buf.pitch(), 0, 0, 0 };
    uint32_t const handles[4] = { buf.handle(), 0, 0, 0 };
    uint32_t const offsets[4] = { 0, 0, 0, 0 };
    uint64_t const modifiers[4] = { DRM_FORMAT_MOD_LINEAR, 0, 0, 0 };
    if (supports_modifiers)
    {
        if (auto err = drmModeAddFB2WithModifiers(
            drm_fd,
            buf.width(),
            buf.height(),
            format,
            handles,
            pitches,
            offsets,
            modifiers,
            &fb_id,
            DRM_MODE_FB_MODIFIERS))
        {
            BOOST_THROW_EXCEPTION((std::system_error{
                -err,
                std::system_category(),
                "Failed to create DRM framebuffer from CPU-accessible buffer"}));
        }
    }
    else
    {
        if (auto err = drmModeAddFB2(
            drm_fd,
            buf.width(),
            buf.height(),
            format,
            handles,
            pitches,
            offsets,
            &fb_id,
            0))
        {
            BOOST_THROW_EXCEPTION((std::system_error{
                -err,
                std::system_category(),
                "Failed to create DRM framebuffer from CPU-accessible buffer"}));
        }


    }
    return fb_id;
}
