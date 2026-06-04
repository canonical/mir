/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "example_xdg_shell_v6.h"

#include "xdg-shell-unstable-v6_wrapper.h"

#include <memory>

namespace
{

class TestXdgToplevel : public mir::wayland::XdgToplevelV6
{
public:
    TestXdgToplevel(struct wl_resource* resource, mir::examples::XdgShellV6CreateCallback cb, wl_resource* surf)
        : mir::wayland::XdgToplevelV6(resource, Version<1>()), callback(std::move(cb))
    {
        struct wl_array empty = {0, 0, nullptr};
        send_configure_event(800, 600, &empty);
        if (callback && surf) callback(wl_resource_get_client(surf), surf);
    }

private:
    void destroy() override {}
    void set_parent(std::optional<struct wl_resource*> const&) override {}
    void set_title(std::string const&) override {}
    void set_app_id(std::string const&) override {}
    void show_window_menu(struct wl_resource*, uint32_t, int32_t, int32_t) override {}
    void move(struct wl_resource*, uint32_t) override {}
    void resize(struct wl_resource*, uint32_t, uint32_t) override {}
    void set_max_size(int32_t, int32_t) override {}
    void set_min_size(int32_t, int32_t) override {}
    void set_maximized() override {}
    void unset_maximized() override {}
    void set_fullscreen(std::optional<struct wl_resource*> const&) override {}
    void unset_fullscreen() override {}
    void set_minimized() override {}

    mir::examples::XdgShellV6CreateCallback callback;
};

class TestXdgSurface : public mir::wayland::XdgSurfaceV6
{
public:
    TestXdgSurface(struct wl_resource* resource, mir::examples::XdgShellV6CreateCallback cb, wl_resource* surf)
        : mir::wayland::XdgSurfaceV6(resource, Version<1>()), callback(std::move(cb)), surface(surf) {}

private:
    void destroy() override {}
    void get_toplevel(struct wl_resource* id) override { new TestXdgToplevel(id, callback, surface); }
    void get_popup(struct wl_resource*, struct wl_resource*, struct wl_resource*) override {}
    void set_window_geometry(int32_t, int32_t, int32_t, int32_t) override {}
    void ack_configure(uint32_t) override {}

    mir::examples::XdgShellV6CreateCallback callback;
    wl_resource* surface;
};

class XdgShellV6Extension final : public mir::wayland::XdgShellV6::Global
{
public:
    XdgShellV6Extension(wl_display* display, mir::examples::XdgShellV6CreateCallback callback)
        : Global(display, Version<1>()), callback(std::move(callback)) {}

private:
    class Instance final : public mir::wayland::XdgShellV6
    {
    public:
        Instance(wl_resource* res, XdgShellV6Extension* ext)
            : mir::wayland::XdgShellV6(res, Version<1>()), ext(ext) {}

    private:
        void destroy() override { delete this; }
        void create_positioner(wl_resource*) override {}
        void get_xdg_surface(wl_resource* id, wl_resource* surface) override
        {
            new TestXdgSurface(id, ext->callback, surface);
        }
        void pong(uint32_t) override {}

        XdgShellV6Extension* ext;
    };

    void bind(wl_resource* r) override { new Instance(r, this); }

    mir::examples::XdgShellV6CreateCallback callback;
};

} // namespace

auto mir::examples::xdg_shell_v6_extension() -> miral::WaylandExtensions::Builder
{
    return xdg_shell_v6_extension([](auto, auto){});
}

auto mir::examples::xdg_shell_v6_extension(XdgShellV6CreateCallback callback)
    -> miral::WaylandExtensions::Builder
{
    return miral::WaylandExtensions::Builder
    {
        mir::wayland::XdgShellV6::interface_name,
        [cb = std::move(callback)](auto const* ctx)
        {
            return std::make_shared<XdgShellV6Extension>(ctx->display(), cb);
        }
    };
}
