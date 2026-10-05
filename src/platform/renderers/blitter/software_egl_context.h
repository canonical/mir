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

#ifndef MIR_RENDERER_BLITTER_EGL_CONTEXT_H_
#define MIR_RENDERER_BLITTER_EGL_CONTEXT_H_

#include <mir/graphics/egl_extensions.h>

#include <EGL/egl.h>
#include <memory>

namespace mir::renderer::blitter
{

/// A surfaceless, software-rastered EGL context.
///
/// If the [blitter::Renderer] fails to blit, it falls back to drawing with GL into
/// framebuffers. Those framebuffers are CPU-addressable scanout buffers, which a
/// real GPU is unlikely to be able to render into, so we deliberately drive the fallback
/// with a software rasteriser.
class SoftwareEGLContext
{
public:
    /// Create and make current a software-rasterised EGL context
    ///
    /// \throws std::runtime_error if no suitable EGL device/context is available
    SoftwareEGLContext();
    ~SoftwareEGLContext();

    SoftwareEGLContext(SoftwareEGLContext const&) = delete;
    auto operator=(SoftwareEGLContext const&) -> SoftwareEGLContext& = delete;

    void make_current() const;
    void release_current() const;
    auto is_current() const -> bool;

    auto display() const -> EGLDisplay { return dpy; }
    auto extensions() const -> graphics::EGLExtensions const& { return *exts; }

private:
    EGLDisplay const dpy;
    EGLContext const ctx;
    std::unique_ptr<graphics::EGLExtensions> const exts;
};

}

#endif // MIR_RENDERER_BLITTER_EGL_CONTEXT_H_
