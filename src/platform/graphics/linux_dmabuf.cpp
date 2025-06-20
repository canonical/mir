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

#include "mir/graphics/linux_dmabuf.h"
#include "mir/anonymous_shm_file.h"
#include "mir/fd.h"
#include "mir/graphics/drm_formats.h"
#include "egl_buffer_copy.h"

#include "wayland_wrapper.h"
#include "mir/wayland/protocol_error.h"
#include "mir/wayland/client.h"
#include "mir/graphics/egl_extensions.h"
#include "mir/graphics/egl_error.h"
#include "mir/graphics/texture.h"
#include "mir/graphics/program_factory.h"
#include "mir/graphics/buffer.h"
#include "mir/graphics/buffer_basic.h"
#include "mir/graphics/dmabuf_buffer.h"
#include "mir/graphics/egl_context_executor.h"

#include <EGL/egl.h>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <sys/stat.h>

#define MIR_LOG_COMPONENT "linux-dmabuf-import"
#include "mir/log.h"

#include <mutex>
#include <vector>
#include <optional>
#include <utility>
#include <algorithm>
#include <drm_fourcc.h>
#include <wayland-server.h>

namespace mg = mir::graphics;
namespace mgc = mg::common;
namespace mw = mir::wayland;
namespace geom = mir::geometry;


namespace
{
    template <typename T>
    void wl_array_add(wl_array *array, T value)
    {
        if (auto d = static_cast<T*>(wl_array_add(array, sizeof(T))))
            *d = value;
    }
}


class mg::DmaBufFormatDescriptors
{
public:
    DmaBufFormatDescriptors(
        EGLDisplay dpy,
        mg::EGLExtensions::EXTImageDmaBufImportModifiers const& dmabuf_ext)
    {
        EGLint num_formats;
        if (dmabuf_ext.eglQueryDmaBufFormatsExt(dpy, 0, nullptr, &num_formats) != EGL_TRUE)
        {
            BOOST_THROW_EXCEPTION((mg::egl_error("Failed to query number of dma-buf formats")));
        }
        if (num_formats < 1)
        {
            BOOST_THROW_EXCEPTION((std::runtime_error{"No dma-buf formats supported"}));
        }
        resize(num_formats);

        EGLint returned_formats;
        if (dmabuf_ext.eglQueryDmaBufFormatsExt(
            dpy, formats.size(), formats.data(), &returned_formats) != EGL_TRUE)
        {
            BOOST_THROW_EXCEPTION((mg::egl_error("Failed to list dma-buf formats")));
        }
        // Paranoia: what if the EGL returns a different number of formats in the second call?
        if (returned_formats != num_formats)
        {
            mir::log_warning(
                "eglQueryDmaBufFormats returned unexpected number of formats (got %i, expected %i)",
                returned_formats,
                num_formats);
            resize(returned_formats);
        }

        for (auto i = 0u; i < formats.size();)
        {
            auto [format, modifiers, external_only] = (*this)[i];

            EGLint num_modifiers;
            if (
                dmabuf_ext.eglQueryDmaBufModifiersExt(
                    dpy,
                    format,
                    0,
                    nullptr,
                    nullptr,
                    &num_modifiers) != EGL_TRUE)
            {
                mir::log_warning("eglQueryDmaBufModifiers failed for format %s: %s",
                    mg::drm_format_to_string(static_cast<uint32_t>(format)),
                    mg::egl_category().message(eglGetError()).c_str());

                // Remove that format and its modifiers from our list
                formats.erase(formats.begin() + i);
                modifiers_for_format.erase(modifiers_for_format.begin() + i);
                external_only_for_format.erase(external_only_for_format.begin() + i);
                // formats[i] is now the format *after* the one we've just removed,
                // so we can can continue iterating through the formats from here.
                continue;
            }
            modifiers.resize(num_modifiers);
            external_only.resize(num_modifiers);

            EGLint returned_modifiers;
            if (
                dmabuf_ext.eglQueryDmaBufModifiersExt(
                    dpy,
                    format,
                    modifiers.size(),
                    modifiers.data(),
                    external_only.data(),
                    &returned_modifiers) != EGL_TRUE)
            {
                BOOST_THROW_EXCEPTION((mg::egl_error("Failed to list modifiers")));
            }
            if (returned_modifiers != num_modifiers)
            {
                mir::log_warning(
                    "eglQueryDmaBufModifiers return unexpected number of modifiers for format 0x%ux"
                    " (expected %i, got %i)",
                    format,
                    returned_modifiers,
                    num_modifiers);
                modifiers.resize(returned_modifiers);
                external_only.resize(returned_modifiers);
            }

            if (num_modifiers == 0)
            {
                /* Special case: num_modifiers == 0 means that the driver doesn't support modifiers;
                 * instead, it will do the right thing if you publish DRM_FORMAT_MOD_INVALID.
                 */
                modifiers.push_back(DRM_FORMAT_MOD_INVALID);
                external_only.push_back(false);
            }

            // We've processed this format; move on to the next one
            ++i;
        }
        if (this->num_formats() == 0)
        {
            /* We can get here if the EGL implementation *claimed* to support some formats,
             * but querying the supported modifiers failed for *all* formats.
             */
            BOOST_THROW_EXCEPTION((std::runtime_error{"EGL claimed support for DMA-Buf import modifiers, but all queries failed"}));
        }
    }

    struct FormatDescriptor
    {
        EGLint format;
        std::vector<EGLuint64KHR> const& modifiers;
        std::vector<EGLBoolean> const& external_only;
    };

    auto num_formats() const -> size_t
    {
        return formats.size();
    }

    auto operator[](size_t idx) const -> FormatDescriptor
    {
        if (idx >= formats.size())
        {
            BOOST_THROW_EXCEPTION((std::out_of_range{
                std::string("Index ") + std::to_string(idx) + " out of bounds (num formats: " +
                std::to_string(formats.size()) + ")"}));
        }

        return FormatDescriptor{
            formats[idx],
            modifiers_for_format[idx],
            external_only_for_format[idx]};
    }


private:
    void resize(size_t size)
    {
        formats.resize(size);
        modifiers_for_format.resize(size);
        external_only_for_format.resize(size);
    }

    auto operator[](size_t idx)
        -> std::tuple<EGLint, std::vector<EGLuint64KHR>&, std::vector<EGLBoolean>&>
    {
        if (idx >= formats.size())
        {
            BOOST_THROW_EXCEPTION((std::out_of_range{
                std::string("Index ") + std::to_string(idx) + " out of bounds (num formats: " +
                std::to_string(formats.size()) + ")"}));
        }
        return std::make_tuple(
            formats[idx],
            std::ref(modifiers_for_format[idx]),
            std::ref(external_only_for_format[idx]));
    }

    std::vector<EGLint> formats;
    std::vector<std::vector<EGLuint64KHR>> modifiers_for_format;
    std::vector<std::vector<EGLBoolean>> external_only_for_format;
};

namespace
{
using PlaneInfo = mg::DMABufBuffer::PlaneDescriptor;

struct BufferGLDescription
{
    GLenum target;
    char const* extension_fragment;
    char const* fragment_fragment;
};

BufferGLDescription const Tex2D = {
    GL_TEXTURE_2D,
    "",
    "uniform sampler2D tex;\n"
    "vec4 sample_to_rgba(in vec2 texcoord)\n"
    "{\n"
    "    return texture2D(tex, texcoord);\n"
    "}\n"
};

BufferGLDescription const ExternalOES = {
    GL_TEXTURE_EXTERNAL_OES,
    "#ifdef GL_ES\n"
    "#extension GL_OES_EGL_image_external : require\n"
    "#endif\n",
    "uniform samplerExternalOES tex;\n"
    "vec4 sample_to_rgba(in vec2 texcoord)\n"
    "{\n"
    "    return texture2D(tex, texcoord);\n"
    "}\n"
};

namespace
{
struct EGLPlaneAttribs
{
    EGLint fd;
    EGLint offset;
    EGLint pitch;
    EGLint modifier_lo;
    EGLint modifier_hi;
};

static constexpr std::array<EGLPlaneAttribs, 4> egl_attribs = {
    EGLPlaneAttribs {
        EGL_DMA_BUF_PLANE0_FD_EXT,
        EGL_DMA_BUF_PLANE0_OFFSET_EXT,
        EGL_DMA_BUF_PLANE0_PITCH_EXT,
        EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT,
        EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT
    },
    EGLPlaneAttribs {
        EGL_DMA_BUF_PLANE1_FD_EXT,
        EGL_DMA_BUF_PLANE1_OFFSET_EXT,
        EGL_DMA_BUF_PLANE1_PITCH_EXT,
        EGL_DMA_BUF_PLANE1_MODIFIER_LO_EXT,
        EGL_DMA_BUF_PLANE1_MODIFIER_HI_EXT
    },
    EGLPlaneAttribs {
        EGL_DMA_BUF_PLANE2_FD_EXT,
        EGL_DMA_BUF_PLANE2_OFFSET_EXT,
        EGL_DMA_BUF_PLANE2_PITCH_EXT,
        EGL_DMA_BUF_PLANE2_MODIFIER_LO_EXT,
        EGL_DMA_BUF_PLANE2_MODIFIER_HI_EXT
    },
    EGLPlaneAttribs {
        EGL_DMA_BUF_PLANE3_FD_EXT,
        EGL_DMA_BUF_PLANE3_OFFSET_EXT,
        EGL_DMA_BUF_PLANE3_PITCH_EXT,
        EGL_DMA_BUF_PLANE3_MODIFIER_LO_EXT,
        EGL_DMA_BUF_PLANE3_MODIFIER_HI_EXT
    }
};

class DMABuf : public mg::DMABufBuffer
{
public:
    DMABuf(
        mg::DRMFormat format,
        std::optional<uint64_t> modifier,
        std::vector<PlaneDescriptor> planes,
        mg::gl::Texture::Layout layout,
        geom::Size size)
        : format_{format},
          modifier_{std::move(modifier)},
          planes_{std::move(planes)},
          layout_{layout},
          size_{size}
    {
    }

    auto format() const -> mg::DRMFormat override
    {
        return format_;
    }
    auto modifier() const -> std::optional<uint64_t> override
    {
        return modifier_;
    }
    auto planes() const -> std::vector<PlaneDescriptor> const& override
    {
        return planes_;
    }
    auto layout() const -> mg::gl::Texture::Layout override
    {
        return layout_;
    }
    auto size() const -> geom::Size override
    {
        return size_;
    }
private:
    mg::DRMFormat const format_;
    std::optional<uint64_t> const modifier_;
    std::vector<PlaneDescriptor> const planes_;
    mg::gl::Texture::Layout const layout_;
    geom::Size const size_;
};

auto export_egl_image(
    mg::EGLExtensions::MESADmaBufExport const& ext,
    EGLDisplay dpy,
    EGLImage image,
    geom::Size size) -> std::unique_ptr<mg::DMABufBuffer>
{
    constexpr int const max_planes = 4;

    int fourcc;
    int num_planes;
    std::array<uint64_t, max_planes> modifiers;
    if (ext.eglExportDMABUFImageQueryMESA(dpy, image, &fourcc, &num_planes, modifiers.data()) != EGL_TRUE)
    {
        BOOST_THROW_EXCEPTION((mg::egl_error("Failed to query EGLImage for dma-buf export")));
    }

    /* There's only a single modifier for a logical buffer. For some reason the EGL interface
     * decided to return one modifier per plane, but they are always the same.
     *
     * We handle DRM_FORMAT_MOD_INVALID as an empty modifier; fix that up here if we get it.
     */
    auto modifier =
        [](uint64_t egl_modifier) -> std::optional<uint64_t>
        {
            if (egl_modifier == DRM_FORMAT_MOD_INVALID)
            {
                return std::nullopt;
            }
            return egl_modifier;
        }(modifiers[0]);

    std::array<int, max_planes> fds;
    std::array<EGLint, max_planes> strides;
    std::array<EGLint, max_planes> offsets;

    if (ext.eglExportDMABUFImageMESA(dpy, image, fds.data(), strides.data(), offsets.data()) != EGL_TRUE)
    {
        BOOST_THROW_EXCEPTION((mg::egl_error("Failed to export EGLImage to dma-buf(s)")));
    }

    std::vector<PlaneInfo> planes;
    planes.reserve(num_planes);
    for (int i = 0; i < num_planes; ++i)
    {
        mir::Fd fd;
        // If multiple planes use the same buffer, the fds array will be filled with -1 for subsequent
        // planes.
        if (fds[i] == -1)
        {
            // Paranoia
            if (i == 0)
            {
                BOOST_THROW_EXCEPTION((std::runtime_error{"Driver has a broken EGL_MESA_image_dma_buf_export extension"}));
            }
            fds[i] = fds[i - 1];
            fd = mir::Fd{mir::IntOwnedFd{fds[i]}};
        }
        else
        {
            // We own these FDs now.
            fd = mir::Fd{fds[i]};
        }
        planes.push_back(
            PlaneInfo {
                .dma_buf = std::move(fd),
                .stride = static_cast<uint32_t>(strides[i]),
                .offset = static_cast<uint32_t>(offsets[i])
            });
    }

    return std::make_unique<DMABuf>(
        mg::DRMFormat{static_cast<uint32_t>(fourcc)},
        modifier,
        std::move(planes),
        mg::gl::Texture::Layout::TopRowFirst,
        size);
}

/**
 * Reimport dmabufs into EGL
 *
 * This is necessary to call each time the buffer is re-submitted by the client,
 * to ensure any state is properly synchronised.
 *
 * \return  An EGLImageKHR handle to the imported
 * \throws  A std::system_error containing the EGL error on failure.
 */
auto import_egl_image(
    int32_t width,
    int32_t height,
    mg::DRMFormat format,
    std::optional<uint64_t> modifier,
    std::vector<PlaneInfo> const& planes,
    EGLDisplay dpy,
    mg::EGLExtensions const& egl_extensions) -> EGLImageKHR
{
    std::vector<EGLint> attributes;

    attributes.push_back(EGL_WIDTH);
    attributes.push_back(width);
    attributes.push_back(EGL_HEIGHT);
    attributes.push_back(height);
    attributes.push_back(EGL_LINUX_DRM_FOURCC_EXT);
    attributes.push_back(format);

    for(auto i = 0u; i < planes.size(); ++i)
    {
        auto const& attrib_names = egl_attribs[i];
        auto const& plane = planes[i];

        attributes.push_back(attrib_names.fd);
        attributes.push_back(static_cast<int>(plane.dma_buf));
        attributes.push_back(attrib_names.offset);
        attributes.push_back(plane.offset);
        attributes.push_back(attrib_names.pitch);
        attributes.push_back(plane.stride);
        if (auto modifier_present = modifier)
        {
            attributes.push_back(attrib_names.modifier_lo);
            attributes.push_back(modifier_present.value() & 0xFFFFFFFF);
            attributes.push_back(attrib_names.modifier_hi);
            attributes.push_back(modifier_present.value() >> 32);
        }
    }
    attributes.push_back(EGL_NONE);
    EGLImage image = egl_extensions.base(dpy).eglCreateImageKHR(
        dpy,
        EGL_NO_CONTEXT,
        EGL_LINUX_DMA_BUF_EXT,
        nullptr,
        attributes.data());

    if (image == EGL_NO_IMAGE_KHR)
    {
        auto const msg = planes.size() > 1 ?
                         "Failed to import supplied dmabufs" :
                         "Failed to import supplied dmabuf";
        BOOST_THROW_EXCEPTION((mg::egl_error(msg)));
    }

    return image;
}

/**
 * Get a description of how to use a specified format/modifier pair in GL
 *
 * \return  A pointer to a statically-allocated descriptor, or nullptr if
 *          this format/modifier pair is invalid.
 */
BufferGLDescription const* descriptor_for_format_and_modifiers(
    mg::DRMFormat format,
    uint64_t modifier,
    mg::DMABufEGLProvider const& provider)
{
    auto const& formats = provider.supported_formats();
    for (auto i = 0u; i < formats.num_formats(); ++i)
    {
        auto const& [supported_format, modifiers, external_only] = formats[i];

        for (auto j = 0u ; j < modifiers.size(); ++j)
        {
            auto const supported_modifier = modifiers[j];

            if (static_cast<uint32_t>(supported_format) == format &&
                modifier == supported_modifier)
            {
                if (external_only[j])
                {
                    return &ExternalOES;
                }
                return &Tex2D;
            }
        }
    }

    /* The specification of zwp_linux_buffer_params_v1.add says:
     *
     *        Warning: It should be an error if the format/modifier pair was not
     *        advertised with the modifier event. This is not enforced yet because
     *        some implementations always accept `DRM_FORMAT_MOD_INVALID`. Also
     *        version 2 of this protocol does not have the modifier event.
     *
     * So don't enforce the error for this case
     */
    if (modifier == DRM_FORMAT_MOD_INVALID)
    {
        return &Tex2D;
    }
    return nullptr;
}

}

/**
 * Holds on to all imported dmabuf buffers, and allows looking up by wl_buffer
 *
 * \note This is not threadsafe, and should only be accessed on the Wayland thread
 */
class WlDmaBufBuffer : public mir::wayland::Buffer, public mir::graphics::DMABufBuffer
{
public:
    WlDmaBufBuffer(
        wl_resource* wl_buffer,
        int32_t width,
        int32_t height,
        mg::DRMFormat format,
        uint32_t flags,
        uint64_t modifier,
        std::vector<PlaneInfo> plane_params)
            : Buffer(wl_buffer, Version<1>{}),
              width{width},
              height{height},
              format_{format},
              flags{flags},
              modifier_{modifier},
              planes_{std::move(plane_params)}
    {
    }

    ~WlDmaBufBuffer() = default;

    static auto maybe_dmabuf_from_wl_buffer(wl_resource* buffer) -> WlDmaBufBuffer*
    {
        return dynamic_cast<WlDmaBufBuffer*>(Buffer::from(buffer));
    }

    auto size() const -> geom::Size override
    {
        return {width, height};
    }

    auto layout() const -> mg::gl::Texture::Layout override
    {
        if (flags & mw::LinuxBufferParamsV1::Flags::y_invert)
        {
            return mg::gl::Texture::Layout::TopRowFirst;
        }
        else
        {
            return mg::gl::Texture::Layout::GL;
        }
    }

    auto format() const -> mg::DRMFormat override
    {
        return format_;
    }

    auto modifier() const -> std::optional<uint64_t> override
    {
        return modifier_;
    }

    auto planes() const -> std::vector<PlaneInfo> const& override
    {
        return planes_;
    }
private:
    int32_t const width, height;
    mg::DRMFormat const format_;
    uint32_t const flags;
    std::optional<uint64_t> const modifier_;
    std::vector<PlaneInfo> const planes_;
};

class LinuxDmaBufParams : public mir::wayland::LinuxBufferParamsV1
{
public:
    LinuxDmaBufParams(
        wl_resource* new_resource,
        std::shared_ptr<mg::DMABufEGLProvider> provider)
        : mir::wayland::LinuxBufferParamsV1(new_resource, Version<5>{}),
          consumed{false},
          provider{std::move(provider)}
    {
    }

private:
    /* EGL_EXT_image_dma_buf_import allows up to 3 planes, and
     * EGL_EXT_image_dma_buf_import_modifiers adds an extra plane.
     */
    std::array<PlaneInfo, 4> planes;
    std::optional<uint64_t> modifier;
    bool consumed;
    /* We don't *really* need to create an mg::Buffer from here, but we *do* need to validate
     * that the dma-buf(s) can be succesfully imported to an EGLImage.
     *
     * The only way to ensure that is to actually import them.
     */
    std::shared_ptr<mg::DMABufEGLProvider> const provider;

    void add(
        mir::Fd fd,
        uint32_t plane_idx,
        uint32_t offset,
        uint32_t stride,
        uint32_t modifier_hi,
        uint32_t modifier_lo) override
    {
        if (consumed)
        {
            BOOST_THROW_EXCEPTION((
                mw::ProtocolError{
                    resource,
                    Error::already_used,
                    "Params already used to create a buffer"}));
        }
        if (plane_idx >= planes.size())
        {
            BOOST_THROW_EXCEPTION((
                mw::ProtocolError{
                    resource,
                    Error::plane_idx,
                    "Plane index %u higher than maximum number of planes, %zu", plane_idx, planes.size()}));
        }

        if (planes[plane_idx].dma_buf != mir::Fd::invalid)
        {
            BOOST_THROW_EXCEPTION((
                mw::ProtocolError{
                    resource,
                    Error::plane_set,
                    "Plane %u already has a dmabuf", plane_idx}));
        }

        planes[plane_idx].dma_buf = std::move(fd);
        planes[plane_idx].offset = offset;
        planes[plane_idx].stride = stride;

        auto const new_modifier = (static_cast<uint64_t>(modifier_hi) << 32) | modifier_lo;
        if (modifier)
        {
            if (*modifier != new_modifier)
            {
                BOOST_THROW_EXCEPTION((
                    mw::ProtocolError{
                        resource,
                        Error::invalid_format,
                        "Modifier %" PRIu64 " for plane %u doesn't match previously set"
                        " modifier %" PRIu64 " - all planes must use the same modifier",
                        new_modifier,
                        plane_idx,
                        *modifier}));
            }
        }
        else
        {
            modifier = (static_cast<uint64_t>(modifier_hi) << 32) | modifier_lo;
        }
    }

    /**
     * Basic sanity check of the set plane properties
     *
     * \throws  A ProtocolError if any sanity checks fail.
     * \return  An iterator pointing to the element past the end of the plane
     *          infos
     */
    auto validate_and_count_planes() -> decltype(LinuxDmaBufParams::planes)::const_iterator
    {
        if (consumed)
        {
            BOOST_THROW_EXCEPTION((
                mw::ProtocolError{
                    resource,
                    Error::already_used,
                    "Params already used to create a buffer"}));
        }
        auto const plane_count =
            std::count_if(
                planes.begin(),
                planes.end(),
                [](auto const& plane) { return plane.dma_buf != mir::Fd::invalid; });
        if (plane_count == 0)
        {
            BOOST_THROW_EXCEPTION((
                mw::ProtocolError{
                    resource,
                    Error::incomplete,
                    "No dmabuf has been added to the params"}));
        }
        for (auto i = 0; i != plane_count; ++i)
        {
            if (planes[i].dma_buf == mir::Fd::invalid)
            {
                BOOST_THROW_EXCEPTION((
                    mw::ProtocolError{
                        resource,
                        Error::incomplete,
                        "Missing dmabuf for plane %u", i}));
            }
        }

        // TODO: Basic verification of size & offset (see libweston/linux-dmabuf.c)
        return planes.cbegin() + plane_count;
    }

    void validate_params(int32_t width, int32_t height, uint32_t format, uint32_t /*flags*/)
    {
        // TODO: Validate flags
        if (width < 1 || height < 1)
        {
            BOOST_THROW_EXCEPTION((
                mw::ProtocolError{
                    resource,
                    Error::invalid_dimensions,
                    "Width %i or height %i invalid; both must be >= 1!",
                    width, height}));
        }
        if (!descriptor_for_format_and_modifiers(mg::DRMFormat{format}, modifier.value(), *provider))
        {
            BOOST_THROW_EXCEPTION((
                mw::ProtocolError{
                    resource,
                    Error::invalid_format,
                    "Client requested unsupported format/modifier combination %s/%s (%u/%u,%u)",
                    mg::DRMFormat{format}.name(),
                    mg::drm_modifier_to_string(*modifier).c_str(),
                    static_cast<uint32_t>(format),
                    static_cast<uint32_t>(*modifier >> 32),
                    static_cast<uint32_t>(*modifier & 0xFFFFFFFF)}));
        }
    }

    void create(int32_t width, int32_t height, uint32_t format, uint32_t flags) override
    {
        validate_params(width, height, format, flags);

        try
        {
            auto const buffer_resource = wl_resource_create(client->raw_client(), &wl_buffer_interface, 1, 0);
            if (!buffer_resource)
            {
                wl_client_post_no_memory(client->raw_client());
                return;
            }

            auto const last_valid_plane = validate_and_count_planes();

            mg::DRMFormat const drm_format{format};
            auto dma_buf = new WlDmaBufBuffer{
                buffer_resource,
                width,
                height,
                drm_format,
                flags,
                modifier.value(),
                {planes.cbegin(), last_valid_plane}};

            // We don't need to keep it around, but we do need to ensure that we *can* create a Buffer
            // from this dma-buf
            provider->validate_import(*dma_buf);
            send_created_event(buffer_resource);
        }
        catch (std::system_error const& err)
        {
            if (err.code().category() != mg::egl_category())
            {
                throw;
            }
            /* The client should handle this fine, but let's make sure we can see
             * any failures that might happen.
             */
            mir::log_debug("Failed to import client dmabufs: %s", err.what());
            send_failed_event();
        }
        consumed = true;
    }

    void
    create_immed(
        struct wl_resource* buffer_id,
        int32_t width,
        int32_t height,
        uint32_t format,
        uint32_t flags) override
    {
        validate_params(width, height, format, flags);

        try
        {
            auto const last_valid_plane = validate_and_count_planes();

            mg::DRMFormat const drm_format{format};
            auto dma_buf = new WlDmaBufBuffer{
                buffer_id,
                width,
                height,
                drm_format,
                flags,
                modifier.value(),
                {planes.cbegin(), last_valid_plane}};
            // We don't need to keep it around, but we do need to ensure that we *can* create a Buffer
            // from this dma-buf
            provider->validate_import(*dma_buf);
        }
        catch (std::system_error const& err)
        {
            if (err.code().category() != mg::egl_category())
            {
                throw;
            }
            /* The protocol gives implementations the choice of sending an invalid_wl_buffer
             * protocol error and disconnecting the client here, or sending a failed() event
             * and not disconnecting the client.
             *
             * Both Weston and GNOME Shell choose to disconnect the client, rather than possibly
             * allowing them to attempt to use an invalid wl_buffer. Let's follow their lead.
             */
            BOOST_THROW_EXCEPTION((
                mw::ProtocolError{
                    resource,
                    Error::invalid_wl_buffer,
                    "Failed to import dmabuf: %s", err.what()
                }));
        }
    }
};

class LinuxDmaBufFeedback : public mir::wayland::LinuxDmabufFeedbackV1
{
public:
    LinuxDmaBufFeedback(
        wl_resource* new_resource,
        struct wl_resource* surface,
        std::shared_ptr<mg::DMABufEGLProvider> provider)
        : mir::wayland::LinuxDmabufFeedbackV1(new_resource, Version<5>{}),
          provider{std::move(provider)}
    {
        // We have the same feedback regardless of surface.
        (void)surface;

        // Build table in shared memory.
        struct Format
        {
            uint32_t format;
            uint32_t padding; /* unused */
            uint64_t modifier;
        };
        auto const& formats = this->provider->supported_formats();
        size_t format_table_length = 0;
        for (auto i = 0u; i < formats.num_formats(); ++i)
        {
            format_table_length += formats[i].modifiers.size();
        }
        mir::AnonymousShmFile shm_buffer{format_table_length * sizeof(Format)};
        Format *data = static_cast<Format*>(shm_buffer.base_ptr());
        size_t k = 0;
        for (auto i = 0u; i < formats.num_formats(); ++i)
        {
            auto format = formats[i];
            for (auto j = 0u; j < format.modifiers.size(); ++j)
            {
                data[k].format = format.format;
                data[k].padding = 0;
                data[k].modifier = format.modifiers[j];
                k++;
            }
        }

        send_format_table_event(mir::Fd{mir::IntOwnedFd{shm_buffer.fd()}}, format_table_length * sizeof(Format));
        wl_array main_device;
        wl_array_init(&main_device);
        wl_array_add<dev_t>(&main_device, this->provider->devnum());
        send_main_device_event(&main_device);

        // We only currently support one device, which accessess all formats.
        wl_array device;
        wl_array_init(&device);
        wl_array_add<dev_t>(&device, this->provider->devnum());
        send_tranche_target_device_event(&device);
        send_tranche_flags_event(0);
        wl_array indicies;
        wl_array_init(&indicies);
        for (auto i = 0u; i < format_table_length; ++i)
        {
            uint32_t *index = static_cast<uint32_t*>(wl_array_add(&indicies, sizeof(uint32_t)));
            *index = i;
        }
        send_tranche_formats_event(&indicies);
        send_tranche_done_event();

        send_done_event();
    }

private:
    std::shared_ptr<mg::DMABufEGLProvider> const provider;
};

GLuint get_tex_id()
{
    GLuint tex;
    glGenTextures(1, &tex);
    return tex;
}

class DMABufTex : public mg::gl::Texture
{
public:
    DMABufTex(
        EGLDisplay dpy,
        mg::EGLExtensions const& extensions,
        mg::DMABufBuffer const& dma_buf,
        BufferGLDescription const& descriptor,
        std::shared_ptr<mgc::EGLContextExecutor> egl_delegate)
        : tex{get_tex_id()},
          desc{descriptor},
          layout_{dma_buf.layout()},
          egl_delegate{std::move(egl_delegate)}
    {
        eglBindAPI(EGL_OPENGL_ES_API);

        auto const target = descriptor.target;

        EGLImage image = import_egl_image(
            dma_buf.size().width.as_int(),
            dma_buf.size().height.as_int(),
            dma_buf.format(),
            dma_buf.modifier(),
            dma_buf.planes(),
            dpy,
            extensions);

        glBindTexture(target, tex);
        extensions.base(dpy).glEGLImageTargetTexture2DOES(target, image);

        // tex is now an EGLImage sibling, so we can free the EGLImage without
        // freeing the backing data.
        extensions.base(dpy).eglDestroyImageKHR(dpy, image);

        glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    ~DMABufTex() override
    {
        egl_delegate->spawn(
            [tex = tex]()
            {
                glDeleteTextures(1, &tex);
            });
    }

    mg::gl::Program const& shader(mg::gl::ProgramFactory& cache) const override
    {
        /* We rely on the fact that `desc` is a reference to a statically-allocated namespaced
         * variable, and so taking the address will give us the address of the static instance,
         * making the cache compile only once for each `desc`.
         */
        return cache.compile_fragment_shader(
            &desc,
            desc.extension_fragment,
            desc.fragment_fragment);
    }

    Layout layout() const override
    {
        return layout_;
    }

    void bind() override
    {
        glBindTexture(desc.target, tex);
    }

    auto tex_id() const -> GLuint override
    {
        return tex;
    }

    void add_syncpoint() override
    {
    }
private:
    GLuint const tex;
    BufferGLDescription const& desc;
    Layout const layout_;
    std::shared_ptr<mgc::EGLContextExecutor> const egl_delegate;
};

namespace
{
auto format_has_known_alpha(mg::DRMFormat format) -> std::optional<bool>
{
    return format.info().transform([](auto const& info) { return info.has_alpha(); });
}
}

class DmabufTexBuffer :
    public mg::BufferBasic,
    public mg::DMABufBuffer
{
public:
    // Note: Must be called with a current EGL context
    DmabufTexBuffer(
        EGLDisplay dpy,
        mg::EGLExtensions const& extensions,
        mg::DMABufBuffer const& dma_buf,
        BufferGLDescription const& descriptor,
        std::shared_ptr<mg::DMABufEGLProvider> provider,
        std::shared_ptr<mgc::EGLContextExecutor> egl_delegate,
        std::function<void()>&& on_consumed,
        std::function<void()>&& on_release)
        : dpy{dpy},
          tex{dpy, extensions, dma_buf, descriptor, std::move(egl_delegate)},
          provider_{std::move(provider)},
          on_consumed_{std::move(on_consumed)},
          on_release{std::move(on_release)},
          size_{dma_buf.size()},
          has_alpha{format_has_known_alpha(dma_buf.format()).value_or(true)},  // Has-alpha is the safe default for unknown formats
          planes_{dma_buf.planes()},
          modifier_{dma_buf.modifier()},
          format_{dma_buf.format()}
    {
    }

    ~DmabufTexBuffer() override
    {
        on_release();
    }

    auto on_same_egl_display(EGLDisplay dpy) -> bool
    {
        return this->dpy == dpy;
    }

    mir::geometry::Size size() const override
    {
        return size_;
    }

    MirPixelFormat pixel_format() const override
    {
        if (auto mir_format = format_.as_mir_format())
        {
            return mir_format.value();
        }
        // There's no way to implement this corectly…
        if (has_alpha)
        {
            return mir_pixel_format_argb_8888;
        }
        else
        {
            return mir_pixel_format_xrgb_8888;
        }
    }

    NativeBufferBase* native_buffer_base() override
    {
        return this;
    }

    void on_consumed() override
    {
        on_consumed_();
        on_consumed_ = [](){};
    }

    auto as_texture() -> DMABufTex*
    {
        /* We only get asked for a texture when the Renderer is about to
         * texture from this buffer; it's a good indication that the buffer
         * has been consumed.
         */
        std::lock_guard lock{consumed_mutex};
        on_consumed();

        return &tex;
    }

    auto format() const -> mg::DRMFormat override
    {
        return format_;
    }

    auto modifier() const -> std::optional<uint64_t> override
    {
        return modifier_;
    }

    auto planes() const -> std::vector<PlaneDescriptor> const& override
    {
        return planes_;
    }

    auto layout() const -> mg::gl::Texture::Layout override
    {
        return tex.layout();
    }

    auto provider() const -> std::shared_ptr<mg::DMABufEGLProvider>
    {
        return provider_;
    }
private:
    EGLDisplay const dpy;
    DMABufTex tex;

    std::shared_ptr<mg::DMABufEGLProvider> const provider_;

    std::mutex consumed_mutex;
    std::function<void()> on_consumed_;
    std::function<void()> const on_release;

    geom::Size const size_;
    bool const has_alpha;

    std::vector<mg::DMABufBuffer::PlaneDescriptor> const planes_;
    std::optional<uint64_t> const modifier_;
    mg::DRMFormat const format_;
};


}

class mg::LinuxDmaBuf::Instance : public mir::wayland::LinuxDmabufV1
{
public:
    Instance(
        wl_resource* new_resource,
        std::shared_ptr<mg::DMABufEGLProvider> provider)
        : mir::wayland::LinuxDmabufV1(new_resource, Version<5>{}),
          provider{std::move(provider)}
    {
        if (wl_resource_get_version(new_resource) < 4)
        {
            send_formats();
        }
    }
private:
    void send_formats()
    {
        auto const& formats = this->provider->supported_formats();
        for (auto i = 0u; i < formats.num_formats(); ++i)
        {
            auto [format, modifiers, external_only] = formats[i];

            send_format_event(format);
            for (auto j = 0u; j < modifiers.size(); ++j)
            {
                auto const modifier = modifiers[j];
                send_modifier_event_if_supported(
                    format,
                    modifier >> 32,
                    modifier & 0xFFFFFFFF);
            }
        }
    }

    void create_params(struct wl_resource* params_id) override
    {
        new LinuxDmaBufParams{params_id, provider};
    }

    void get_default_feedback(struct wl_resource* params_id) override
    {
        new LinuxDmaBufFeedback{params_id, nullptr, provider};
    }

    void get_surface_feedback(struct wl_resource* params_id, struct wl_resource* surface) override
    {
        new LinuxDmaBufFeedback{params_id, surface, provider};
    }

    std::shared_ptr<mg::DMABufEGLProvider> const provider;
};

mg::LinuxDmaBuf::LinuxDmaBuf(
    wl_display* display,
    std::shared_ptr<mg::DMABufEGLProvider> provider)
    : mir::wayland::LinuxDmabufV1::Global(display, Version<5>{}),
      provider{std::move(provider)}
{
}

auto mg::LinuxDmaBuf::buffer_from_resource(
    wl_resource* buffer,
    std::function<void()>&& on_consumed,
    std::function<void()>&& on_release,
    std::shared_ptr<mgc::EGLContextExecutor> /*egl_delegate*/)
    -> std::shared_ptr<Buffer>
{
    if (auto dmabuf = WlDmaBufBuffer::maybe_dmabuf_from_wl_buffer(buffer))
    {
        return provider->import_dma_buf(
            *dmabuf,
            std::move(on_consumed),
            std::move(on_release));
    }
    return nullptr;
}

void mg::LinuxDmaBuf::bind(wl_resource* new_resource)
{
    new LinuxDmaBuf::Instance{new_resource, provider};
}

namespace
{
dev_t get_devnum(EGLDisplay dpy)
{
    try
    {
        mg::EGLExtensions::DeviceQuery device_query_ext;
        EGLDeviceEXT device;
        device_query_ext.eglQueryDisplayAttribEXT(dpy, EGL_DEVICE_EXT, reinterpret_cast<EGLAttrib*>(&device));
        const char *device_path = device_query_ext.eglQueryDeviceStringEXT(device, EGL_DRM_DEVICE_FILE_EXT);
        if (device_path == nullptr)
        {
            mir::log_info(
                "Unable to determine linux-dmabuf device: no device path returned from EGL");
            return 0;
        }
        struct stat device_stat;
        if (stat(device_path, &device_stat) == -1)
        {
            mir::log_info(
                "Unable to determine linux-dmabuf device: unable to stat device path: %s", strerror(errno));
            return 0;
        }

        return device_stat.st_rdev;
    }
    catch (std::runtime_error const& error)
    {
        mir::log_info(
            "Unable to determine linux-dmabuf device: %s", error.what());
        return 0;
    }
}
}

mg::DMABufEGLProvider::DMABufEGLProvider(
    EGLDisplay dpy,
    std::shared_ptr<EGLExtensions> egl_extensions,
    mg::EGLExtensions::EXTImageDmaBufImportModifiers const& dmabuf_ext,
    std::shared_ptr<mgc::EGLContextExecutor> egl_delegate,
    EGLImageAllocator allocate_importable_image)
    : dpy{dpy},
      egl_extensions{std::move(egl_extensions)},
      dmabuf_export_ext{mg::EGLExtensions::MESADmaBufExport::extension_if_supported(dpy)},
      devnum_{get_devnum(dpy)},
      formats{std::make_unique<DmaBufFormatDescriptors>(dpy, dmabuf_ext)},
      egl_delegate{std::move(egl_delegate)},
      allocate_importable_image{std::move(allocate_importable_image)},
      blitter{std::make_unique<mg::EGLBufferCopier>(this->egl_delegate)}
{
}

mg::DMABufEGLProvider::~DMABufEGLProvider() = default;

auto mg::DMABufEGLProvider::devnum() const -> dev_t
{
    return devnum_;
}

auto mg::DMABufEGLProvider::supported_formats() const -> mg::DmaBufFormatDescriptors const&
{
    return *formats;
}

auto mg::DMABufEGLProvider::import_dma_buf(
    mg::DMABufBuffer const& dma_buf,
    std::function<void()>&& on_consumed,
    std::function<void()>&& on_release) -> std::shared_ptr<Buffer>
{
    // This will have been pre-validated at point the Wayland client created the buffer
    auto descriptor = descriptor_for_format_and_modifiers(
        dma_buf.format(),
        dma_buf.modifier().value_or(DRM_FORMAT_MOD_INVALID),
        *this);
    return std::make_shared<DmabufTexBuffer>(
        dpy,
        *egl_extensions,
        dma_buf,
        *descriptor,
        shared_from_this(),
        egl_delegate,
        std::move(on_consumed),
        std::move(on_release));
}

void mg::DMABufEGLProvider::validate_import(DMABufBuffer const& dma_buf)
{
    auto image = import_egl_image(
        dma_buf.size().width.as_int(), dma_buf.size().height.as_int(),
        dma_buf.format(),
        dma_buf.modifier(),
        dma_buf.planes(),
        dpy,
        *egl_extensions);
    if (image != EGL_NO_IMAGE_KHR)
    {
        // We can throw this image away immediately
        egl_extensions->base(dpy).eglDestroyImageKHR(dpy, image);
    }
}

auto mg::DMABufEGLProvider::as_texture(std::shared_ptr<NativeBufferBase> buffer)
    -> std::shared_ptr<gl::Texture>
{
    if (auto dmabuf_tex = std::dynamic_pointer_cast<DmabufTexBuffer>(buffer))
    {
        if (dmabuf_tex->on_same_egl_display(dpy))
        {
            auto tex = dmabuf_tex->as_texture();
            return std::shared_ptr<gl::Texture>(std::move(dmabuf_tex), tex);
        }
        else if (auto descriptor = descriptor_for_format_and_modifiers(
                    dmabuf_tex->format(),
                    dmabuf_tex->modifier().value_or(DRM_FORMAT_MOD_INVALID),
                    *this))
        {
            // Cross-GPU import requires explicit modifiers; MOD_INVALID will not work
            if (dmabuf_tex->modifier().value_or(DRM_FORMAT_MOD_INVALID) != DRM_FORMAT_MOD_INVALID)
            {
                /* We're being naughty here and using the fact that `as_texture()` has a side-effect
                 * of invoking the buffer's `on_consumed()` callback.
                 */
                dmabuf_tex->as_texture();
                return std::make_shared<DMABufTex>(
                    dpy,
                    *egl_extensions,
                    *dmabuf_tex,
                    *descriptor,
                    egl_delegate);
            }
        }
        /* Oh, no. We've got a dma-buf in a format that our rendering GPU can't handle.
         *
         * In this case we'll need to get the *importing* GPU to blit to a format
         * we *can* handle.
         */
        auto importing_provider = dmabuf_tex->provider();

        if (!importing_provider->dmabuf_export_ext)
        {
            mir::log_warning("EGL implementation does not handle cross-GPU buffer export");
            return nullptr;
        }

        /* TODO: Be smarter about finding a shared pixel format; everything *should* do
         * ARGB8888, but if the buffer is in a higher bitdepth this will lose colour information
         */
        auto const& supported_formats = *formats;
        auto const& modifiers =
            [&supported_formats]() -> std::vector<uint64_t> const&
            {
                for (size_t i = 0; i < supported_formats.num_formats(); ++i)
                {
                    if (supported_formats[i].format == DRM_FORMAT_ARGB8888)
                    {
                        return supported_formats[i].modifiers;
                    }
                }
                BOOST_THROW_EXCEPTION((std::runtime_error{"Platform doesn't support ARGB8888?!"}));
            }();

        auto importable_buf = importing_provider->allocate_importable_image(
            mg::DRMFormat{DRM_FORMAT_ARGB8888},
            std::span<uint64_t const>{modifiers.data(), modifiers.size()},
            dmabuf_tex->size());

        if (!importable_buf)
        {
            mir::log_warning("Failed to allocate common-format buffer for cross-GPU buffer import");
            return nullptr;
        }

        auto src_image = import_egl_image(
            dmabuf_tex->size().width.as_int(), dmabuf_tex->size().height.as_int(),
            dmabuf_tex->format(),
            dmabuf_tex->modifier(),
            dmabuf_tex->planes(),
            importing_provider->dpy,
            *importing_provider->egl_extensions);
        auto importable_image = import_egl_image(
            importable_buf->size().width.as_int(), importable_buf->size().height.as_int(),
            importable_buf->format(),
            importable_buf->modifier(),
            importable_buf->planes(),
            importing_provider->dpy,
            *importing_provider->egl_extensions);
        auto sync = importing_provider->blitter->blit(src_image, importable_image, dmabuf_tex->size());
        if (sync)
        {
            BOOST_THROW_EXCEPTION((std::logic_error{"EGL_ANDROID_native_fence_sync support not implemented yet"}));
        }
        auto importable_dmabuf = export_egl_image(*importing_provider->dmabuf_export_ext, importing_provider->dpy, importable_image, dmabuf_tex->size());

        auto base_extension = importing_provider->egl_extensions->base(importing_provider->dpy);
        base_extension.eglDestroyImageKHR(importing_provider->dpy, src_image);
        base_extension.eglDestroyImageKHR(importing_provider->dpy, importable_image);

        if (auto descriptor = descriptor_for_format_and_modifiers(
                    importable_dmabuf->format(),
                    importable_dmabuf->modifier().value_or(DRM_FORMAT_MOD_INVALID),
                    *this))
        {
            /* We're being naughty here and using the fact that `as_texture()` has a side-effect
             * of invoking the buffer's `on_consumed()` callback.
             */
            dmabuf_tex->as_texture();
            return std::make_shared<DMABufTex>(
                dpy,
                *egl_extensions,
                *importable_dmabuf,
                *descriptor,
                egl_delegate);
        }

        /* To get here we have to have failed to find the format/modifier descriptor for a
         * buffer that we've explicitly allocated to be importable by us.
         *
         * This is a logic bug, so go noisily.
         */
        BOOST_THROW_EXCEPTION((std::logic_error{"Failed to find import parameterns for buffer we explicitly allocated for import"}));
    }
    return nullptr;
}
