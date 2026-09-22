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

#ifndef MIR_GRAPHICS_EGL_DMABUF_IMPORT_H_
#define MIR_GRAPHICS_EGL_DMABUF_IMPORT_H_

#include <mir/geometry/size.h>
#include <mir/graphics/dmabuf_buffer.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <cstdint>
#include <optional>
#include <span>

namespace mir::graphics
{
class DRMFormat;
class EGLExtensions;

/**
 * Import dma-buf(s) into EGL as an EGLImage
 *
 * This is necessary to call each time a buffer is re-submitted by the client,
 * to ensure any state is properly synchronised.
 *
 * \return  An EGLImageKHR handle to the imported buffer
 * \throws  A std::system_error containing the EGL error on failure.
 */
auto import_dmabuf_to_egl_image(
    EGLDisplay dpy,
    EGLExtensions const& egl_extensions,
    geometry::Size size,
    DRMFormat format,
    std::optional<uint64_t> modifier,
    std::span<DMABufBuffer::PlaneDescriptor const> planes) -> EGLImageKHR;

/// Import a whole DMABufBuffer into EGL as an EGLImage
auto import_dmabuf_to_egl_image(
    EGLDisplay dpy,
    EGLExtensions const& egl_extensions,
    DMABufBuffer const& buffer) -> EGLImageKHR;

}

#endif // MIR_GRAPHICS_EGL_DMABUF_IMPORT_H_
