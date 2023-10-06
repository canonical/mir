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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "make_shm_pool.h"
#include "xdg-shell.h"
#include "mir-shell.h"

#include <wayland-client.h>
#include <wayland-client-core.h>

#include <memory>
#include <signal.h>
#include <string.h>

namespace
{
int const pixel_size = 4;
int const no_of_buffers = 4;

auto const display = wl_display_connect(NULL);

namespace globals
{
wl_compositor* compositor;
wl_shm* shm;
wl_seat* seat;
wl_output* output;
xdg_wm_base* wm_base;
zmir_mir_shell_v1* mir_shell;

void init();
}

void handle_registry_global(
    wl_registry* registry,
    uint32_t id,
    char const* interface,
    uint32_t version)
{
    if (strcmp(interface, "wl_compositor") == 0)
    {
        globals::compositor = static_cast<wl_compositor*>(wl_registry_bind(
            registry, id, &wl_compositor_interface, std::min(version, 3u)));
    }
    else if (strcmp(interface, "wl_shm") == 0)
    {
        globals::shm = static_cast<wl_shm*>(wl_registry_bind(registry, id, &wl_shm_interface, std::min(version, 1u)));
        // Normally we'd add a listener to pick up the supported formats here
        // As luck would have it, I know that argb8888 is the only format we support :)
    }
    else if (strcmp(interface, "wl_seat") == 0)
    {
        globals::seat = static_cast<wl_seat*>(wl_registry_bind(registry, id, &wl_seat_interface, std::min(version, 4u)));
    }
    else if (strcmp(interface, "wl_output") == 0)
    {
        globals::output = static_cast<wl_output*>(wl_registry_bind(
            registry, id, &wl_output_interface, std::min(version, 2u)));
    }
    else if (strcmp(interface, xdg_wm_base_interface.name) == 0)
    {
        globals::wm_base = static_cast<xdg_wm_base*>(wl_registry_bind(
            registry, id, &xdg_wm_base_interface, std::min(version, 1u)));
    }
    else if (strcmp(interface, zmir_mir_shell_v1_interface.name) == 0)
    {
        globals::mir_shell = static_cast<zmir_mir_shell_v1*>(wl_registry_bind(
            registry, id, &zmir_mir_shell_v1_interface, std::min(version, 1u)));
    }
}

wl_registry_listener const registry_listener =
{
    .global = [](void*, auto... args) { handle_registry_global(args...); },
    .global_remove = [](auto...) {},
};

xdg_wm_base_listener const shell_listener =
{
    .ping = [](void*, xdg_wm_base* shell, uint32_t serial) { xdg_wm_base_pong(shell, serial); }
};

void globals::init()
{
    auto const registry = wl_display_get_registry(display);

    wl_registry_add_listener(registry, &registry_listener, NULL);

    wl_display_roundtrip(display);

    bool fail = false;
    if (!compositor)
    {
        puts("ERROR: no wl_compositor*");
        fail = true;
    }
    if (!shm)
    {
        puts("ERROR: no wl_shm*");
        fail = true;
    }
    if (!seat)
    {
        puts("ERROR: no wl_seat*");
        fail = true;
    }
    if (!output)
    {
        puts("ERROR: no wl_output*");
        fail = true;
    }
    if (!wm_base)
    {
        puts("ERROR: no wm_base*");
        fail = true;
    }
    if (!mir_shell)
    {
        puts("WARNING: no mir_shell*");
    }

    if (fail) abort();

    xdg_wm_base_add_listener(globals::wm_base, &shell_listener, NULL);
}

class window
{
public:
    window(int32_t width, int32_t height);
    virtual ~window() = default;

protected:
    struct buffer
    {
        wl_buffer* buffer;
        bool available;
        int width;
        int height;
        void* content_area;
    };
    
private:
    buffer buffers[no_of_buffers];
    wl_surface* surface;
    int width;
    int height;
    bool waiting_for_buffer = false;
    bool need_to_draw = true;

    void handle_frame_callback(wl_callback* callback, uint32_t);
    void update_free_buffers(wl_buffer* buffer);
    void prepare_buffer(buffer& b);
    void handle_xdg_toplevel_configure(xdg_toplevel*, int32_t width_, int32_t height_, wl_array*);
    auto find_free_buffer() -> buffer*;
    virtual void draw_new_content(buffer* b) = 0;

    static xdg_toplevel_listener const shell_toplevel_listener;
    static wl_callback_listener const frame_listener;
    static wl_buffer_listener const buffer_listener;
    static xdg_surface_listener const shell_surface_listener;

    void fake_frame();

    window(window const&) = delete;
    window& operator=(window const&) = delete;
};

class pulsing_window : public window
{
public:
    using window::window;

private:
    unsigned char current_intensity = 128;
    void draw_new_content(buffer* b) override;
};

xdg_toplevel_listener const window::shell_toplevel_listener =
{
    .configure = [](void* ctx, auto... args) { static_cast<window*>(ctx)->handle_xdg_toplevel_configure(args...); },
    .close = [](auto...){},
    .configure_bounds = [](auto...){},
    .wm_capabilities = [](auto...){},
};

wl_callback_listener const window::frame_listener =
{
    .done = [](void* ctx, auto... args) { static_cast<window*>(ctx)->handle_frame_callback(args...); },
};

wl_buffer_listener const window::buffer_listener =
{
    .release = [](void* ctx, auto... args) { static_cast<window*>(ctx)->update_free_buffers(args...); },
};

xdg_surface_listener const window::shell_surface_listener =
{
    .configure= [](auto...){},
};

void window::update_free_buffers(wl_buffer* buffer)
{
    for (int i = 0; i < no_of_buffers; ++i)
    {
        if (buffers[i].buffer == buffer)
        {
            buffers[i].available = true;
        }
    }

    if (waiting_for_buffer)
    {
        waiting_for_buffer = false;
        fake_frame();
    }
}

void window::prepare_buffer(buffer& b)
{
    void* pool_data = NULL;
    wl_shm_pool* shm_pool = make_shm_pool(globals::shm, width * height * pixel_size, &pool_data);

    b.buffer = wl_shm_pool_create_buffer(shm_pool, 0, width, height, width * pixel_size, WL_SHM_FORMAT_ARGB8888);
    b.available = true;
    b.width = width;
    b.height = height;
    b.content_area = pool_data;
    wl_buffer_add_listener(b.buffer, &buffer_listener, this);
    wl_shm_pool_destroy(shm_pool);
}

auto window::find_free_buffer() -> buffer*
{
    for (auto& b : buffers)
    {
        if (b.available)
        {
            if (b.width != width || b.height != height)
            {
                wl_buffer_destroy(b.buffer);
                prepare_buffer(b);
            }

            b.available = false;
            return &b;
        }
    }
    return nullptr;
}

void window::handle_frame_callback(wl_callback* callback, uint32_t)
{
    wl_callback_destroy(callback);

    if (need_to_draw)
    {
        if (auto const b = find_free_buffer())
        {
            draw_new_content(b);

            auto const new_frame_signal = wl_surface_frame(surface);
            wl_callback_add_listener(new_frame_signal, &frame_listener, this);
            wl_surface_attach(surface, b->buffer, 0, 0);
            wl_surface_commit(surface);
            need_to_draw = false;

            return;
        }
        else
        {
            waiting_for_buffer = true;
        }
    }

    fake_frame();
}

void pulsing_window::draw_new_content(buffer* b)
{
    memset(b->content_area, current_intensity, b->width * b->height * pixel_size);
    ++current_intensity;
}

volatile sig_atomic_t running = 0;

void shutdown(int signum)
{
    if (running)
    {
        running = 0;
        printf("Signal %d received. Good night.\n", signum);
    }
}

void window::handle_xdg_toplevel_configure(
    xdg_toplevel*,
    int32_t width_,
    int32_t height_,
    wl_array*)
{
    if (width_ > 0) width = width_;
    if (height_ > 0) height = height_;

    if ((width_ > 0) || (height_ > 0))
    {
        need_to_draw = true;
    }
}

void window::fake_frame()
{
    wl_callback* fake_frame = wl_display_sync(display);
    wl_callback_add_listener(fake_frame, &frame_listener, this);
}

window::window(int32_t width, int32_t height) :
    surface{wl_compositor_create_surface(globals::compositor)},
    width{width},
    height{height}
{
    for (buffer& b : buffers)
    {
        prepare_buffer(b);
    }

    auto const shell_surface = xdg_wm_base_get_xdg_surface(globals::wm_base, surface);
    xdg_surface_add_listener(shell_surface, &shell_surface_listener, NULL);

    auto const shell_toplevel = xdg_surface_get_toplevel(shell_surface);
    xdg_toplevel_add_listener(shell_toplevel, &shell_toplevel_listener, this);

    fake_frame();
}
}

int main()
{
    globals::init();

    auto const main_window = std::make_unique<pulsing_window>(400, 400);

    struct sigaction sig_handler_new;
    sigfillset(&sig_handler_new.sa_mask);
    sig_handler_new.sa_flags = 0;
    sig_handler_new.sa_handler = shutdown;

    sigaction(SIGINT, &sig_handler_new, NULL);
    sigaction(SIGTERM, &sig_handler_new, NULL);
    sigaction(SIGHUP, &sig_handler_new, NULL);

    running = 1;

    while (wl_display_dispatch(display) && running)
        ;

    return 0;
}
