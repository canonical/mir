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

#include "egl_dmabuf_import.h"

#include <mir/graphics/drm_formats.h>
#include <mir/graphics/egl_error.h>
#include <mir/graphics/egl_extensions.h>

#include <boost/throw_exception.hpp>
#include <array>
#include <vector>

namespace mg = mir::graphics;
namespace geom = mir::geometry;

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

constexpr std::array<EGLPlaneAttribs, 4> egl_attribs = {
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
}

auto mg::import_dmabuf_to_egl_image(
    EGLDisplay dpy,
    EGLExtensions const& egl_extensions,
    geom::Size size,
    DRMFormat format,
    std::optional<uint64_t> modifier,
    std::span<DMABufBuffer::PlaneDescriptor const> planes) -> EGLImageKHR
{
    std::vector<EGLint> attributes;

    attributes.push_back(EGL_WIDTH);
    attributes.push_back(size.width.as_int());
    attributes.push_back(EGL_HEIGHT);
    attributes.push_back(size.height.as_int());
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

auto mg::import_dmabuf_to_egl_image(
    EGLDisplay dpy,
    EGLExtensions const& egl_extensions,
    DMABufBuffer const& buffer) -> EGLImageKHR
{
    return import_dmabuf_to_egl_image(
        dpy,
        egl_extensions,
        buffer.size(),
        buffer.format(),
        buffer.modifier(),
        buffer.planes());
}
