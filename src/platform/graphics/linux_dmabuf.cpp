/*
 * Copyright © 2020 Canonical Ltd.
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


#include "mir/graphics/linux_dmabuf.h"
#include "mir/graphics/drm_formats.h"


#include "wayland_wrapper.h"
#include "mir/graphics/egl_extensions.h"
#include "mir/graphics/egl_error.h"
#include "mir/renderer/gl/context.h"
#include "mir/graphics/texture.h"
#include "mir/graphics/program_factory.h"
#include "mir/graphics/buffer.h"
#include "mir/graphics/buffer_basic.h"
#include "mir/graphics/dmabuf_buffer.h"
#include "mir/executor.h"

#define MIR_LOG_COMPONENT "linux-dmabuf-import"
#include "mir/log.h"


#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <mutex>
#include <vector>
#include <optional>
#include <drm_fourcc.h>
#include <wayland-server.h>

namespace mg = mir::graphics;
namespace mw = mir::wayland;
namespace geom = mir::geometry;


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

        for (auto i = 0u; i < formats.size(); ++i)
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
                BOOST_THROW_EXCEPTION((mg::egl_error("Failed to query number of modifiers")));
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

/**
 * Holds on to all imported dmabuf buffers, and allows looking up by wl_buffer
 *
 * \note This is not threadsafe, and should only be accessed on the Wayland thread
 */
class WlDmaBufBuffer : public mir::wayland::Buffer
{
public:
    WlDmaBufBuffer(
        EGLDisplay dpy,
        std::shared_ptr<mg::EGLExtensions> egl_extensions,
        BufferGLDescription const& desc,
        wl_resource* wl_buffer,
        int32_t width,
        int32_t height,
        mg::DRMFormat format,
        uint32_t flags,
        uint64_t modifier,
        std::vector<PlaneInfo> plane_params)
            : Buffer(wl_buffer, Version<1>{}),
              dpy{dpy},
              egl_extensions{std::move(egl_extensions)},
              desc{desc},
              width{width},
              height{height},
              format_{format},
              flags{flags},
              modifier_{modifier},
              planes_{std::move(plane_params)},
              image{EGL_NO_IMAGE_KHR}
    {
        reimport_egl_image();
    }

    ~WlDmaBufBuffer()
    {
        if (image != EGL_NO_IMAGE_KHR)
        {
            egl_extensions->base(dpy).eglDestroyImageKHR(dpy, image);
        }
    }

    static auto maybe_dmabuf_from_wl_buffer(wl_resource* buffer) -> WlDmaBufBuffer*
    {
        return dynamic_cast<WlDmaBufBuffer*>(Buffer::from(buffer));
    }

    auto size() -> geom::Size
    {
        return {width, height};
    }

    auto layout() -> mg::gl::Texture::Layout
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

    auto format() -> uint32_t
    {
        return format_;
    }

    auto descriptor() const -> BufferGLDescription const&
    {
        return desc;
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
    auto reimport_egl_image() -> EGLImageKHR
    {
        std::vector<EGLint> attributes;

        attributes.push_back(EGL_WIDTH);
        attributes.push_back(width);
        attributes.push_back(EGL_HEIGHT);
        attributes.push_back(height);
        attributes.push_back(EGL_LINUX_DRM_FOURCC_EXT);
        attributes.push_back(format());

        for(auto i = 0u; i < planes_.size(); ++i)
        {
            auto const& attrib_names = egl_attribs[i];
            auto const& plane = planes()[i];

            attributes.push_back(attrib_names.fd);
            attributes.push_back(static_cast<int>(plane.dma_buf));
            attributes.push_back(attrib_names.offset);
            attributes.push_back(plane.offset);
            attributes.push_back(attrib_names.pitch);
            attributes.push_back(plane.stride);
            if (modifier() != DRM_FORMAT_MOD_INVALID)
            {
                attributes.push_back(attrib_names.modifier_lo);
                attributes.push_back(modifier() & 0xFFFFFFFF);
                attributes.push_back(attrib_names.modifier_hi);
                attributes.push_back(modifier() >> 32);
            }
        }
        attributes.push_back(EGL_NONE);
        if (image != EGL_NO_IMAGE_KHR)
        {
            egl_extensions->base(dpy).eglDestroyImageKHR(dpy, image);
        }
        image = egl_extensions->base(dpy).eglCreateImageKHR(
            dpy,
            EGL_NO_CONTEXT,
            EGL_LINUX_DMA_BUF_EXT,
            nullptr,
            attributes.data());

        if (image == EGL_NO_IMAGE_KHR)
        {
            auto const msg = planes_.size() > 1 ?
                "Failed to import supplied dmabufs" :
                "Failed to import supplied dmabuf";
            BOOST_THROW_EXCEPTION((mg::egl_error(msg)));
        }

        return image;
    }

    auto modifier() -> uint64_t
    {
        return modifier_;
    }

    auto planes() -> std::vector<PlaneInfo> const&
    {
        return planes_;
    }
private:
    EGLDisplay const dpy;
    std::shared_ptr<mg::EGLExtensions> const egl_extensions;
    BufferGLDescription const& desc;
    int32_t const width, height;
    mg::DRMFormat const format_;
    uint32_t const flags;
    uint64_t const modifier_;
    std::vector<PlaneInfo> const planes_;
    EGLImageKHR image;

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
};

class LinuxDmaBufParams : public mir::wayland::LinuxBufferParamsV1
{
public:
    LinuxDmaBufParams(
        wl_resource* new_resource,
        EGLDisplay dpy,
        std::shared_ptr<mg::EGLExtensions> egl_extensions,
        std::shared_ptr<mg::DmaBufFormatDescriptors const> formats)
        : mir::wayland::LinuxBufferParamsV1(new_resource, Version<3>{}),
          consumed{false},
          dpy{dpy},
          egl_extensions{std::move(egl_extensions)},
          formats{std::move(formats)}
    {
    }

private:
    /* EGL_EXT_image_dma_buf_import allows up to 3 planes, and
     * EGL_EXT_image_dma_buf_import_modifiers adds an extra plane.
     */
    std::array<PlaneInfo, 4> planes;
    std::optional<uint64_t> modifier;
    bool consumed;
    EGLDisplay dpy;
    std::shared_ptr<mg::EGLExtensions> egl_extensions;
    std::shared_ptr<mg::DmaBufFormatDescriptors const> const formats;

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

    void validate_params(int32_t width, int32_t height, uint32_t /*format*/, uint32_t /*flags*/)
    {
        if (width < 1 || height < 1)
        {
            BOOST_THROW_EXCEPTION((
                mw::ProtocolError{
                    resource,
                    Error::invalid_dimensions,
                    "Width %i or height %i invalid; both must be >= 1!",
                    width, height}));

            // TODO: Validate format & flags
        }
    }

    BufferGLDescription const& descriptor_for_format_and_modifiers(mg::DRMFormat format)
    {
        /* The optional<uint64_t> modifier is guaranteed to be engaged here,
         * as the add() call fills it if it is unset, and validate_and_count_planes()
         * has already checked that add() has been called at least once.
         */
        auto const requested_modifier = modifier.value();
        for (auto i = 0u; i < formats->num_formats(); ++i)
        {
            auto const& [supported_format, modifiers, external_only] = (*formats)[i];

            for (auto j = 0u ; j < modifiers.size(); ++j)
            {
                auto const supported_modifier = modifiers[j];

                if (static_cast<uint32_t>(supported_format) == format &&
                    requested_modifier == supported_modifier)
                {
                    if (external_only[j])
                    {
                        return ExternalOES;
                    }
                    return Tex2D;
                }
            }
        }

        // The specification of zwp_linux_buffer_params_v1.add says:
        //
        //        Warning: It should be an error if the format/modifier pair was not
        //        advertised with the modifier event. This is not enforced yet because
        //        some implementations always accept `DRM_FORMAT_MOD_INVALID`. Also
        //        version 2 of this protocol does not have the modifier event.
        //
        // So don't enforce the error for this case
        if (requested_modifier == DRM_FORMAT_MOD_INVALID)
        {
            return Tex2D;
        }

        BOOST_THROW_EXCEPTION((
            mw::ProtocolError{
                resource,
                Error::invalid_format,
                "Client requested unsupported format/modifier combination %s/%s (%u/%u,%u)",
                format.name(),
                mg::drm_modifier_to_string(requested_modifier).c_str(),
                static_cast<uint32_t>(format),
                static_cast<uint32_t>(requested_modifier >> 32),
                static_cast<uint32_t>(requested_modifier & 0xFFFFFFFF)}));
    }

    void create(int32_t width, int32_t height, uint32_t format, uint32_t flags) override
    {
        validate_params(width, height, format, flags);

        try
        {
            auto const buffer_resource = wl_resource_create(client, &wl_buffer_interface, 1, 0);
            if (!buffer_resource)
            {
                wl_client_post_no_memory(client);
                return;
            }

            auto const last_valid_plane = validate_and_count_planes();

            mg::DRMFormat const drm_format{format};
            new WlDmaBufBuffer{
                dpy,
                egl_extensions,
                descriptor_for_format_and_modifiers(drm_format),
                buffer_resource,
                width,
                height,
                drm_format,
                flags,
                modifier.value(),
                {planes.cbegin(), last_valid_plane}};
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
            new WlDmaBufBuffer{
                dpy,
                egl_extensions,
                descriptor_for_format_and_modifiers(drm_format),
                buffer_id,
                width,
                height,
                drm_format,
                flags,
                modifier.value(),
                {planes.cbegin(), last_valid_plane}};
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

GLuint get_tex_id()
{
    GLuint tex;
    glGenTextures(1, &tex);
    return tex;
}

bool drm_format_has_alpha(uint32_t format)
{
    /* TODO: We should really have something like libweston/pixel-formats.h
     * We've said this multiple times before, so 🤷
     */

    switch (format)
    {
        /* This is only an optimisation, so pick a bunch of formats and hope that
         * covers it…
         */
        case DRM_FORMAT_XBGR2101010:
        case DRM_FORMAT_BGRX1010102:
        case DRM_FORMAT_XBGR8888:
        case DRM_FORMAT_BGRX8888:
        case DRM_FORMAT_XRGB2101010:
        case DRM_FORMAT_RGBX1010102:
        case DRM_FORMAT_RGBX8888:
        case DRM_FORMAT_XRGB8888:
            return false;
        default:
        // TODO: I *think* true is a safe default, as it'll just mean we're blending something without alpha?
            return true;
    }
}

class WaylandDmabufTexBuffer :
    public mg::BufferBasic,
    public mg::gl::Texture,
    public mg::DMABufBuffer
{
public:
    // Note: Must be called with a current EGL context
    WaylandDmabufTexBuffer(
        WlDmaBufBuffer& source,
        mg::EGLExtensions const& extensions,
        std::shared_ptr<mir::renderer::gl::Context> ctx,
        EGLDisplay dpy,
        std::function<void()>&& on_consumed,
        std::function<void()>&& on_release,
        std::shared_ptr<mir::Executor> wayland_executor)
        : ctx{std::move(ctx)},
          tex{get_tex_id()},
          desc{source.descriptor()},
          on_consumed{std::move(on_consumed)},
          on_release{std::move(on_release)},
          size_{source.size()},
          layout_{source.layout()},
          has_alpha{drm_format_has_alpha(source.format())},
          planes_{source.planes()},
          modifier_{source.modifier()},
          fourcc{source.format()},
          wayland_executor{std::move(wayland_executor)}
    {
        eglBindAPI(EGL_OPENGL_ES_API);

        auto const target = source.descriptor().target;

        glBindTexture(target, tex);
        extensions.base(dpy).glEGLImageTargetTexture2DOES(target, source.reimport_egl_image());
        // tex is now an EGLImage sibling, so we can free the EGLImage without
        // freeing the backing data.

        glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    ~WaylandDmabufTexBuffer() override
    {
        wayland_executor->spawn(
            [context = ctx, tex = tex]()
            {
              context->make_current();

              glDeleteTextures(1, &tex);

              context->release_current();
            });

        on_release();
    }

    mir::geometry::Size size() const override
    {
        return size_;
    }

    MirPixelFormat pixel_format() const override
    {
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

    mir::graphics::gl::Program const& shader(mir::graphics::gl::ProgramFactory& cache) const override
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

        std::lock_guard<decltype(consumed_mutex)> lock(consumed_mutex);
        on_consumed();
        on_consumed = [](){};
    }

    void add_syncpoint() override
    {
    }

    auto drm_fourcc() const -> uint32_t override
    {
        return fourcc;
    }

    auto modifier() const -> std::optional<uint64_t> override
    {
        return modifier_;
    }

    auto planes() const -> std::vector<PlaneDescriptor> const& override
    {
        return planes_;
    }

private:
    std::shared_ptr<mir::renderer::gl::Context> const ctx;
    GLuint const tex;
    BufferGLDescription const& desc;

    std::mutex consumed_mutex;
    std::function<void()> on_consumed;
    std::function<void()> const on_release;

    geom::Size const size_;
    Layout const layout_;
    bool const has_alpha;

    std::vector<mg::DMABufBuffer::PlaneDescriptor> const planes_;
    std::optional<uint64_t> const modifier_;
    uint32_t const fourcc;

    std::shared_ptr<mir::Executor> const wayland_executor;
};


}

class mg::LinuxDmaBufUnstable::Instance : public mir::wayland::LinuxDmabufV1
{
public:
    Instance(
        wl_resource* new_resource,
        EGLDisplay dpy,
        std::shared_ptr<EGLExtensions> egl_extensions,
        std::shared_ptr<DmaBufFormatDescriptors const> formats)
        : mir::wayland::LinuxDmabufV1(new_resource, Version<3>{}),
          dpy{dpy},
          egl_extensions{std::move(egl_extensions)},
          formats{std::move(formats)}
    {
        for (auto i = 0u; i < this->formats->num_formats(); ++i)
        {
            auto [format, modifiers, external_only] = (*(this->formats))[i];

            send_format_event(format);
            if (version_supports_modifier())
            {
                for (auto j = 0u; j < modifiers.size(); ++j)
                {
                    auto const modifier = modifiers[j];
                    send_modifier_event(
                        format,
                        modifier >> 32,
                        modifier & 0xFFFFFFFF);
                }
            }
        }
    }
private:
    void create_params(struct wl_resource* params_id) override
    {
        new LinuxDmaBufParams{params_id, dpy, egl_extensions, formats};
    }

    EGLDisplay const dpy;
    std::shared_ptr<EGLExtensions> const egl_extensions;
    std::shared_ptr<DmaBufFormatDescriptors const> const formats;
};

mg::LinuxDmaBufUnstable::LinuxDmaBufUnstable(
    wl_display* display,
    EGLDisplay dpy,
    std::shared_ptr<EGLExtensions> egl_extensions,
    EGLExtensions::EXTImageDmaBufImportModifiers const& dmabuf_ext)
    : mir::wayland::LinuxDmabufV1::Global(display, Version<3>{}),
      dpy{dpy},
      egl_extensions{std::move(egl_extensions)},
      formats{std::make_shared<DmaBufFormatDescriptors>(dpy, dmabuf_ext)}
{
}

auto mg::LinuxDmaBufUnstable::buffer_from_resource(
    wl_resource* buffer,
    std::shared_ptr<renderer::gl::Context> ctx,
    std::function<void()>&& on_consumed,
    std::function<void()>&& on_release,
    std::shared_ptr<Executor> wayland_executor)
    -> std::shared_ptr<Buffer>
{
    if (auto dmabuf = WlDmaBufBuffer::maybe_dmabuf_from_wl_buffer(buffer))
    {
        return std::make_shared<WaylandDmabufTexBuffer>(
            *dmabuf,
            *egl_extensions,
            std::move(ctx),
            dpy,
            std::move(on_consumed),
            std::move(on_release),
            std::move(wayland_executor));
    }
    return nullptr;
}

void mg::LinuxDmaBufUnstable::bind(wl_resource* new_resource)
{
    new LinuxDmaBufUnstable::Instance{new_resource, dpy, egl_extensions, formats};
}
