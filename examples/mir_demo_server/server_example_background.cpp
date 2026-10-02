/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "server_example_background.h"

#include "wayland_app.h"
#include "wayland_shm.h"
#include "wlr-layer-shell-unstable-v1.h"

#include <mir/fd.h>

#include <boost/throw_exception.hpp>

#include <algorithm>
#include <climits>
#include <cstring>
#include <map>
#include <system_error>

#include <poll.h>
#include <sys/eventfd.h>

namespace me = mir::examples;
namespace geom = mir::geometry;

namespace
{
uint32_t const background_colour = 0xff000000;

class BackgroundApp;

class BackgroundSurface
{
public:
    BackgroundSurface(BackgroundApp& app, WaylandOutput const* output);
    ~BackgroundSurface();

    void draw();

private:
    static zwlr_layer_surface_v1_listener const layer_surface_listener;

    BackgroundApp& app;
    WaylandOutput const* const output;
    WaylandShm shm;
    WaylandObject<wl_surface> const surface;
    WaylandObject<zwlr_layer_surface_v1> const layer_surface;
    geom::Size size;
};

class BackgroundApp : public WaylandApp
{
public:
    explicit BackgroundApp(wl_display* display);
    ~BackgroundApp();

    auto layer_shell() const -> zwlr_layer_shell_v1* { return layer_shell_; }

private:
    void output_ready(WaylandOutput const* output) override;
    void output_changed(WaylandOutput const* output) override;
    void output_gone(WaylandOutput const* output) override;

    WaylandObject<zwlr_layer_shell_v1> layer_shell_;
    std::map<WaylandOutput const*, std::unique_ptr<BackgroundSurface>> surfaces;
};

BackgroundSurface::BackgroundSurface(BackgroundApp& app, WaylandOutput const* output) :
    app{app},
    output{output},
    shm{app.shm()},
    surface{wl_compositor_create_surface(app.compositor()), wl_surface_destroy},
    layer_surface{
        zwlr_layer_shell_v1_get_layer_surface(
            app.layer_shell(), surface, output->wl(), ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND, "background"),
        zwlr_layer_surface_v1_destroy}
{
    zwlr_layer_surface_v1_add_listener(layer_surface, &layer_surface_listener, this);
    zwlr_layer_surface_v1_set_anchor(
        layer_surface,
        ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
        ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT);
    zwlr_layer_surface_v1_set_exclusive_zone(layer_surface, -1);
    wl_surface_commit(surface);
}

BackgroundSurface::~BackgroundSurface() = default;

void BackgroundSurface::draw()
{
    if (size.width.as_int() == 0 || size.height.as_int() == 0)
    {
        return;
    }

    auto const scale = output->scale();
    geom::Size const buffer_size{size.width.as_int() * scale, size.height.as_int() * scale};
    geom::Stride const stride{buffer_size.width.as_int() * 4};
    auto const buffer = shm.get_buffer(buffer_size, stride);
    std::fill_n(
        static_cast<uint32_t*>(buffer->data()),
        buffer_size.width.as_int() * buffer_size.height.as_int(),
        background_colour);

    wl_surface_set_buffer_scale(surface, scale);
    wl_surface_attach(surface, buffer->use(), 0, 0);
    wl_surface_damage(surface, 0, 0, INT32_MAX, INT32_MAX);
    wl_surface_commit(surface);
}

zwlr_layer_surface_v1_listener const BackgroundSurface::layer_surface_listener = {
    [](void* data, zwlr_layer_surface_v1* layer_surface, uint32_t serial, uint32_t width, uint32_t height)
    {
        auto const self = static_cast<BackgroundSurface*>(data);
        zwlr_layer_surface_v1_ack_configure(layer_surface, serial);
        self->size = geom::Size{width, height};
        self->draw();
    },
    [](void*, zwlr_layer_surface_v1*) {},
};

BackgroundApp::BackgroundApp(wl_display* display)
{
    // Bind the layer shell before WaylandApp reports the outputs
    static wl_registry_listener const registry_listener = {
        [](void* data, wl_registry* registry, uint32_t id, char const* interface, uint32_t version)
        {
            if (std::string_view(interface) == zwlr_layer_shell_v1_interface.name)
            {
                *static_cast<WaylandObject<zwlr_layer_shell_v1>*>(data) = {
                    static_cast<zwlr_layer_shell_v1*>(
                        wl_registry_bind(registry, id, &zwlr_layer_shell_v1_interface, std::min(version, 3u))),
                    [](zwlr_layer_shell_v1* layer_shell)
                    {
                        if (zwlr_layer_shell_v1_get_version(layer_shell) >= ZWLR_LAYER_SHELL_V1_DESTROY_SINCE_VERSION)
                        {
                            zwlr_layer_shell_v1_destroy(layer_shell);
                        }
                        else
                        {
                            wl_proxy_destroy(reinterpret_cast<wl_proxy*>(layer_shell));
                        }
                    }};
            }
        },
        [](void*, wl_registry*, uint32_t) {},
    };
    WaylandObject<wl_registry> const registry{wl_display_get_registry(display), wl_registry_destroy};
    wl_registry_add_listener(registry, &registry_listener, &layer_shell_);
    wl_display_roundtrip(display);

    wayland_init(display);
}

BackgroundApp::~BackgroundApp()
{
    surfaces.clear();
}

void BackgroundApp::output_ready(WaylandOutput const* output)
{
    if (layer_shell_)
    {
        surfaces[output] = std::make_unique<BackgroundSurface>(*this, output);
    }
}

void BackgroundApp::output_changed(WaylandOutput const* output)
{
    if (auto const surface = surfaces.find(output); surface != surfaces.end())
    {
        surface->second->draw();
    }
}

void BackgroundApp::output_gone(WaylandOutput const* output)
{
    surfaces.erase(output);
}
}

struct me::Background::Self
{
    Self() :
        shutdown_signal{eventfd(0, EFD_CLOEXEC)}
    {
        if (shutdown_signal == mir::Fd::invalid)
        {
            BOOST_THROW_EXCEPTION((
                std::system_error{errno, std::system_category(), "Failed to create shutdown notifier"}));
        }
    }

    void run(wl_display* display)
    {
        BackgroundApp app{display};

        enum FdIndices { display_fd, shutdown, indices };
        pollfd fds[indices];
        fds[display_fd] = {wl_display_get_fd(display), POLLIN, 0};
        fds[shutdown] = {shutdown_signal, POLLIN, 0};

        while (!(fds[shutdown].revents & (POLLIN | POLLERR)))
        {
            while (wl_display_prepare_read(display) != 0)
            {
                if (wl_display_dispatch_pending(display) == -1)
                {
                    BOOST_THROW_EXCEPTION((
                        std::system_error{errno, std::system_category(), "Failed to dispatch Wayland events"}));
                }
            }

            wl_display_flush(display);

            if (poll(fds, indices, -1) == -1)
            {
                wl_display_cancel_read(display);
                BOOST_THROW_EXCEPTION((
                    std::system_error{errno, std::system_category(), "Failed to wait for event"}));
            }

            if (fds[display_fd].revents & (POLLIN | POLLERR))
            {
                if (wl_display_read_events(display))
                {
                    BOOST_THROW_EXCEPTION((
                        std::system_error{errno, std::system_category(), "Failed to read Wayland events"}));
                }
            }
            else
            {
                wl_display_cancel_read(display);
            }
        }
    }

    void stop()
    {
        if (eventfd_write(shutdown_signal, 1) == -1)
        {
            BOOST_THROW_EXCEPTION((
                std::system_error{errno, std::system_category(), "Failed to stop background client"}));
        }
    }

    mir::Fd const shutdown_signal;
};

me::Background::Background() :
    self{std::make_shared<Self>()},
    client{
        [self = self](wl_display* display) { self->run(display); },
        [](std::weak_ptr<mir::scene::Session> const) {},
        [self = self] { self->stop(); }}
{
}

void me::Background::operator()(mir::Server& server)
{
    client(server);
}
