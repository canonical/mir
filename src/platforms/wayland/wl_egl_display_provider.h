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

#ifndef MIR_PLATFORM_WAYLAND_DISPLAY_PROVIDER_H_
#define MIR_PLATFORM_WAYLAND_DISPLAY_PROVIDER_H_

#include "mir/graphics/platform.h"

#include <wayland-client.h>

struct wl_egl_window;

namespace mir::graphics::wayland
{

class WlDisplayProvider : public GenericEGLDisplayProvider
{
public:
    WlDisplayProvider(EGLDisplay dpy);

    WlDisplayProvider(
        WlDisplayProvider const& from,
        struct wl_surface* surface,
        geometry::Size size);

    auto get_egl_display() -> EGLDisplay override;

    auto describe_platform() const -> std::string override;
private:
    EGLDisplay const dpy;
};

class WlDisplayAllocator : public GenericEGLDisplayAllocator
{
public:
    WlDisplayAllocator(EGLDisplay dpy, struct wl_surface* surface, geometry::Size size);
    ~WlDisplayAllocator();

    auto alloc_framebuffer(GLConfig const& config, EGLContext share_context)
        -> std::unique_ptr<EGLFramebuffer> override;

    auto describe_platform() const -> std::string override;

    class Framebuffer : public GenericEGLDisplayAllocator::EGLFramebuffer
    {
    public:
        /**
         * Handle for an EGL output surface
         *
         * \note    This takes ownership of \param ctx and \param surf; when the
         *          final handle generated from this Framebuffer is released,
         *          the EGL resources \param ctx and \param surff will be freed.
         */
        Framebuffer(EGLDisplay dpy, EGLContext ctx, EGLSurface surf, geometry::Size size);

        auto size() const -> geometry::Size override;

        void make_current() override;
        void release_current() override;
        auto clone_handle() -> std::unique_ptr<GenericEGLDisplayAllocator::EGLFramebuffer> override;

        void swap_buffers();
    private:
        class EGLState;
        Framebuffer(std::shared_ptr<EGLState const> surf, geometry::Size size);

        std::shared_ptr<EGLState const> const state;
        geometry::Size const size_;
    };
private:
    EGLDisplay const dpy;
    struct ::wl_egl_window* const wl_window;
    geometry::Size const size;
};
}

#endif // MIR_PLATFORM_WAYLAND_DISPLAY_PROVIDER_H_
