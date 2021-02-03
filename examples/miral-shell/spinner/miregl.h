/*
 * Copyright © 2015-2018 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef UNITYSYSTEMCOMPOSITOR_MIREGL_H
#define UNITYSYSTEMCOMPOSITOR_MIREGL_H

#include "wayland_surface.h"

#include <EGL/egl.h>

#include <memory>
#include <vector>

class MirEglApp;
class MirEglSurface;

std::shared_ptr<MirEglApp> make_mir_eglapp(struct wl_display* display);
std::vector<std::shared_ptr<MirEglSurface>> mir_surface_init(std::shared_ptr<MirEglApp> const& app);

class MirEglSurface : WaylandSurface
{
public:
    MirEglSurface(std::shared_ptr<MirEglApp> const& mir_egl_app, struct wl_output* wl_output);

    ~MirEglSurface();

    template<typename Painter>
    void paint(Painter const& functor)
    {
        egl_make_current();
        functor(width_, height_);
        swap_buffers();
    }

private:
    void egl_make_current();

    void swap_buffers();

    std::shared_ptr<MirEglApp> const mir_egl_app;

    void* content_area = nullptr;
    struct wl_callback* new_frame_signal = nullptr;
    struct Buffers
    {
        struct wl_buffer* buffer;
        bool available;
    } buffers[4];
    bool waiting_for_buffer = true;

    EGLSurface eglsurface;
    int width_{0};
    int height_{0};

    static void shell_surface_ping(void *data, struct wl_shell_surface *wl_shell_surface, uint32_t serial);

    static void shell_surface_configure(void *data,
        struct wl_shell_surface *wl_shell_surface,
        uint32_t edges,
        int32_t width,
        int32_t height);
    static void shell_surface_popup_done(void *data, struct wl_shell_surface *wl_shell_surface);

};

#endif //UNITYSYSTEMCOMPOSITOR_MIREGL_H
