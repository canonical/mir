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
#include "wayland_runner.h"
#include "xdg-shell.h"
#include "mir-shell.h"

#include <wayland-client.h>
#include <wayland-client-core.h>

#include <X11/X.h>
#include <getopt.h>
#include <linux/input-event-codes.h>

#include <memory>
#include <string.h>

using namespace std::literals;

namespace
{
bool const tracing = getenv("MIR_SHELL_DEMO_TRACE");

int32_t main_width = 400;
int32_t main_height = 400;

bool satellite_enabled = false;
xdg_positioner_anchor satellite_anchor = XDG_POSITIONER_ANCHOR_LEFT;
int32_t satellite_offset_vertical = 50;
int32_t satellite_offset_horizontal = 0;

void parse_options(int argc, char* argv[])
{
    auto const option_main_width = "width";
    auto const option_main_height = "height";
    auto const option_satellite = "satellite";
    auto const option_satellite_anchor = "satellite-anchor";
    auto const option_satellite_offset_vertical = "satellite-offset-vertical";
    auto const option_satellite_offset_horizontal = "satellite-offset-horizontal";

    struct option const long_options[] = {
        {option_main_width,       required_argument, 0, 0 },
        {option_main_height,      required_argument, 0, 0 },
        {option_satellite,        no_argument,       0, 0 },
        {option_satellite_anchor, required_argument, 0, 0 },
        {option_satellite_offset_vertical, required_argument, 0, 0 },
        {option_satellite_offset_horizontal, required_argument, 0, 0 },
        {0,                 0,                       0, 0 }
    };

    int option_index = 0;

    while_has_no_init_statement:
    if (auto c = getopt_long(argc, argv, "", long_options, &option_index); c != -1)
    {

        switch (c)
        {
        case 0:
            if (option_satellite == long_options[option_index].name)
            {
                satellite_enabled = true;
            }
            else if (option_satellite_anchor == long_options[option_index].name)
            {
                if ("right"s == optarg)
                {
                    satellite_enabled = true;
                    satellite_anchor = XDG_POSITIONER_ANCHOR_RIGHT;
                }
            }
            else if (option_satellite_anchor == long_options[option_index].name)
            {
                if ("right"s == optarg)
                {
                    satellite_enabled = true;
                    satellite_anchor = XDG_POSITIONER_ANCHOR_RIGHT;
                }
            }
            else if (option_satellite_offset_vertical == long_options[option_index].name)
            {
                satellite_enabled = true;
                satellite_offset_vertical = atoi(optarg);
            }
            else if (option_satellite_offset_horizontal == long_options[option_index].name)
            {
                satellite_enabled = true;
                satellite_offset_horizontal = atoi(optarg);
            }
            else if (option_main_width == long_options[option_index].name)
            {
                main_width = atoi(optarg);
            }
            else if (option_main_height == long_options[option_index].name)
            {
                main_height = atoi(optarg);
            }
            else
            {
                printf("option %s", long_options[option_index].name);
                if (optarg)
                    printf(" with arg %s", optarg);
                printf("\n");
            }
            break;

        case '?':
            exit(EXIT_FAILURE);
        }

        goto while_has_no_init_statement;
    }
}

[[gnu::format (printf, 1, 2)]]
inline void trace(char const* fmt, ...)
{
    if (tracing)
    {
        va_list va;
        va_start(va, fmt);
        vprintf(fmt, va);
        va_end(va);
        putc('\n', stdout);
        fflush(stdout);
    }
}

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

class window
{
public:
    window(int32_t width, int32_t height);
    virtual ~window();

    operator wl_surface*() const { return surface; }

    auto size() const { return std::tuple(width, height); }

protected:
    struct buffer
    {
        wl_buffer* buffer;
        bool available;
        int width;
        int height;
        void* content_area;
    };

    void resize(int32_t width_, int32_t height_)
    {
        if (width_ > 0) width = width_;
        if (height_ > 0) height = height_;

        if ((width_ > 0) || (height_ > 0))
        {
            need_to_draw = true;
        }
    }

    virtual void handle_mouse_button(wl_pointer*, uint32_t serial, uint32_t time, uint32_t button, uint32_t state);
    virtual void handle_keyboard_key(wl_keyboard* keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state);
    virtual void handle_keyboard_modifiers(
        wl_keyboard* keyboard,
        uint32_t serial,
        uint32_t mods_depressed,
        uint32_t mods_latched,
        uint32_t mods_locked,
        uint32_t group);

    bool has_mouse_focus() { return mouse_focus == this; }
    bool has_keyboard_focus() { return keyboard_focus == this; }

private:
    static window* mouse_focus;
    static window* keyboard_focus;

    wl_surface* const surface;
    buffer buffers[no_of_buffers];
    wl_pointer* const pointer;
    wl_keyboard* const keyboard;
    int width;
    int height;
    bool waiting_for_buffer = false;
    bool need_to_draw = true;

    void handle_frame_callback(wl_callback* callback, uint32_t);

    void handle_mouse_enter(wl_pointer*, uint32_t serial, wl_surface*, wl_fixed_t surface_x, wl_fixed_t surface_y);
    void handle_mouse_leave(wl_pointer*, uint32_t serial, wl_surface*);
    void handle_mouse_motion(wl_pointer*, uint32_t time, wl_fixed_t surface_x, wl_fixed_t surface_y);
    void handle_mouse_axis(wl_pointer*, uint32_t time, uint32_t axis, wl_fixed_t value);

    void handle_keyboard_keymap(wl_keyboard* keyboard, uint32_t format, int32_t fd, uint32_t size);
    void handle_keyboard_enter(wl_keyboard* keyboard, uint32_t serial, wl_surface* surface, wl_array* keys);
    void handle_keyboard_leave(wl_keyboard* keyboard, uint32_t serial, wl_surface* surface);
    void handle_keyboard_repeat_info(wl_keyboard* wl_keyboard, int32_t rate, int32_t delay);

    void update_free_buffers(wl_buffer* buffer);
    void prepare_buffer(buffer& b);
    auto find_free_buffer() -> buffer*;
    virtual void draw_new_content(buffer* b) = 0;

    static wl_callback_listener const frame_listener;
    static wl_buffer_listener const buffer_listener;
    static wl_pointer_listener const pointer_listener;
    static wl_keyboard_listener const keyboard_listener;
    
    void fake_frame();

    window(window const&) = delete;
    window& operator=(window const&) = delete;
};

window* window::mouse_focus = nullptr;
window* window::keyboard_focus = nullptr;

class toplevel : public window
{
public:
    toplevel(int32_t width, int32_t height);
    ~toplevel();

    operator xdg_surface*() const { return xdgsurface; }
    operator xdg_toplevel*() const { return xdgtoplevel; }

private:
    xdg_surface* const xdgsurface;
    xdg_toplevel* const xdgtoplevel;

    void handle_mouse_button(wl_pointer*, uint32_t serial, uint32_t time, uint32_t button, uint32_t state) override;
    void handle_xdg_toplevel_configure(xdg_toplevel*, int32_t width_, int32_t height_, wl_array*);

    static xdg_toplevel_listener const shell_toplevel_listener;
    static xdg_surface_listener const shell_surface_listener;
};

class grey_window : public toplevel
{
public:
    grey_window(int32_t width, int32_t height, unsigned char intensity) : toplevel(width, height), intensity(intensity) {}

private:
    unsigned char const intensity;
    unsigned char const alpha = 255;
    void draw_new_content(buffer* b) override;
};


class normal_window : public grey_window
{
public:
    normal_window(int32_t width, int32_t height);
    ~normal_window();

protected:
    void
    handle_keyboard_key(wl_keyboard* keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state) override;

    void handle_keyboard_modifiers(
        wl_keyboard* keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked,
        uint32_t group) override;

private:
    zmir_mir_normal_surface_v1* const mir_normal_surface;

    uint32_t modifiers = 0;
};

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

void globals::init()
{
    static wl_registry_listener const registry_listener =
        {
            .global = [](void*, auto... args) { handle_registry_global(args...); },
            .global_remove = [](auto...) {},
        };

    static xdg_wm_base_listener const shell_listener =
        {
            .ping = [](void*, xdg_wm_base* shell, uint32_t serial) { xdg_wm_base_pong(shell, serial); }
        };

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

// If building against newer Wayland protocol definitions we may miss trailing fields
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
wl_callback_listener const window::frame_listener =
{
    .done = [](void* ctx, auto... args) { static_cast<window*>(ctx)->handle_frame_callback(args...); },
};

wl_buffer_listener const window::buffer_listener =
{
    .release = [](void* ctx, auto... args) { static_cast<window*>(ctx)->update_free_buffers(args...); },
};


wl_pointer_listener const window::pointer_listener =
{
    .enter  = [](void* ctx, auto... args) { static_cast<window*>(ctx)->handle_mouse_enter(args...); },
    .leave  = [](void* ctx, auto... args) { static_cast<window*>(ctx)->handle_mouse_leave(args...); },
    .motion = [](void* ctx, auto... args) { static_cast<window*>(ctx)->handle_mouse_motion(args...); },
    .button = [](void* ctx, auto... args) { static_cast<window*>(ctx)->handle_mouse_button(args...); },
    .axis   = [](void* ctx, auto... args) { static_cast<window*>(ctx)->handle_mouse_axis(args...); },
};

wl_keyboard_listener const window::keyboard_listener =
{
    [](void* self, auto... args) { static_cast<window*>(self)->handle_keyboard_keymap(args...); },
    [](void* self, auto... args) { static_cast<window*>(self)->handle_keyboard_enter(args...); },
    [](void* self, auto... args) { static_cast<window*>(self)->handle_keyboard_leave(args...); },
    [](void* self, auto... args) { static_cast<window*>(self)->handle_keyboard_key(args...); },
    [](void* self, auto... args) { static_cast<window*>(self)->handle_keyboard_modifiers(args...); },
    [](void* self, auto... args) { static_cast<window*>(self)->handle_keyboard_repeat_info(args...); },
};
#pragma GCC diagnostic pop

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

void window::handle_mouse_enter(
    wl_pointer*,
    uint32_t serial,
    wl_surface* surface,
    wl_fixed_t surface_x,
    wl_fixed_t surface_y)
{
    if (*this == surface)
        mouse_focus = this;

    (void)serial;
    trace("Received handle_mouse_enter event: (%f, %f)",
          wl_fixed_to_double(surface_x),
          wl_fixed_to_double(surface_y));
}

void window::handle_mouse_leave(
    wl_pointer*,
    uint32_t serial,
    wl_surface* surface)
{
    if (*this == surface)
        mouse_focus = nullptr;

    (void)serial;
    trace("Received mouse_exit event\n");
}

void window::handle_mouse_motion(
    wl_pointer*,
    uint32_t time,
    wl_fixed_t surface_x,
    wl_fixed_t surface_y)
{
    trace("Received motion event: (%f, %f) @ %i",
          wl_fixed_to_double(surface_x),
          wl_fixed_to_double(surface_y),
          time);
}

void window::handle_mouse_button(
    wl_pointer*,
    uint32_t serial,
    uint32_t time,
    uint32_t button,
    uint32_t state)
{
    (void)serial;

    trace("Received button event: Button %i, state %i @ %i",
          button,
          state,
          time);
}

void window::handle_mouse_axis(
    wl_pointer*,
    uint32_t time,
    uint32_t axis,
    wl_fixed_t value)
{
    trace("Received axis event: axis %i, value %f @ %i",
          axis,
          wl_fixed_to_double(value),
          time);
}


void window::handle_keyboard_keymap(wl_keyboard*, uint32_t format, int32_t /*fd*/, uint32_t size)
{
    trace("Received keyboard_keymap: format %i, size %i", format, size);
}

void window::handle_keyboard_enter(wl_keyboard*, uint32_t, wl_surface* surface, wl_array*)
{
    if (*this == surface)
        keyboard_focus = this;

    trace("Received keyboard_enter:\n");
}

void window::handle_keyboard_leave(wl_keyboard*, uint32_t, wl_surface* surface)
{
    if (*this == surface)
        keyboard_focus = nullptr;

    trace("Received keyboard_leave:\n");
}

void window::handle_keyboard_key(wl_keyboard*, uint32_t, uint32_t, uint32_t key, uint32_t state)
{
    trace("Received keyboard_key: key %i, state %i", key, state);
}

void window::handle_keyboard_modifiers(
    wl_keyboard*, uint32_t, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked,
    uint32_t group)
{
    trace("Received keyboard_modifiers: depressed %i, latched %i, locked %i, group %i", mods_depressed, mods_latched, mods_locked, group);
}

void window::handle_keyboard_repeat_info(wl_keyboard*, int32_t rate, int32_t delay)
{
    trace("Received keyboard_modifiers: rate %i, delay %i", rate, delay);
}

void window::fake_frame()
{
    wl_callback* fake_frame = wl_display_sync(display);
    wl_callback_add_listener(fake_frame, &frame_listener, this);
}

window::window(int32_t width, int32_t height) :
    surface{wl_compositor_create_surface(globals::compositor)},
    pointer{wl_seat_get_pointer(globals::seat)},
    keyboard{wl_seat_get_keyboard(globals::seat)},
    width{width},
    height{height}
{
    for (buffer& b : buffers)
    {
        prepare_buffer(b);
    }

    wl_keyboard_add_listener(keyboard, &keyboard_listener, this);
    wl_pointer_add_listener(pointer, &pointer_listener, this);

    fake_frame();
}

window::~window()
{
    for (auto b : buffers)
    {
        wl_buffer_destroy(b.buffer);
    }

    wl_pointer_destroy(pointer);
    wl_surface_destroy(surface);
}

toplevel::toplevel(int32_t width, int32_t height) :
    window{width, height},
    xdgsurface{xdg_wm_base_get_xdg_surface(globals::wm_base, *this)},
    xdgtoplevel{xdg_surface_get_toplevel(xdgsurface)}
{
    xdg_surface_add_listener(xdgsurface, &shell_surface_listener, this);
    xdg_toplevel_add_listener(xdgtoplevel, &shell_toplevel_listener, this);
}

toplevel::~toplevel()
{
    xdg_toplevel_destroy(xdgtoplevel);
    xdg_surface_destroy(xdgsurface);
}

void toplevel::handle_xdg_toplevel_configure(
    xdg_toplevel*,
    int32_t width_,
    int32_t height_,
    wl_array*)
{
    trace("Received toplevel_configure: width %i, height %i", width_, height_);
    resize(width_, height_);
}

xdg_toplevel_listener const toplevel::shell_toplevel_listener =
{
    .configure = [](void* ctx, auto... args) { static_cast<toplevel*>(ctx)->handle_xdg_toplevel_configure(args...); },
    .close = [](auto...){},
    .configure_bounds = [](auto...){},
    .wm_capabilities = [](auto...){},
};

xdg_surface_listener const toplevel::shell_surface_listener =
{
    .configure= [](auto...){},
};

void toplevel::handle_mouse_button(wl_pointer* pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state)
{
    window::handle_mouse_button(pointer, serial, time, button, state);

    if (!has_mouse_focus())
        return;

    if (button == BTN_LEFT && state == WL_POINTER_BUTTON_STATE_PRESSED)
        xdg_toplevel_move(xdgtoplevel, globals::seat, serial);

    if (button == BTN_RIGHT && state == WL_POINTER_BUTTON_STATE_PRESSED)
        xdg_toplevel_resize(xdgtoplevel, globals::seat, serial, XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM_RIGHT);
}

void grey_window::draw_new_content(window::buffer* b)
{
    auto const begin = static_cast<uint8_t*>(b->content_area);
    auto const end = begin + b->width * b->height * pixel_size;

    memset(begin, intensity, end - begin);
    for (auto* p = begin; p != end; p += pixel_size)
    {
        p[3] = alpha;
    }
}

normal_window::normal_window(int32_t width, int32_t height) :
    grey_window(width, height, 192),
    mir_normal_surface{globals::mir_shell? zmir_mir_shell_v1_get_normal_surface(globals::mir_shell, *this) : nullptr}
{
    xdg_toplevel_set_title(*this, "normal");
}

normal_window::~normal_window()
{
    if (mir_normal_surface)
    {
        zmir_mir_normal_surface_v1_destroy(mir_normal_surface);
    }
}

void normal_window::handle_keyboard_key(wl_keyboard* keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
{
    grey_window::handle_keyboard_key(keyboard, serial, time, key, state);

    if (modifiers == ControlMask && key == KEY_Q)
    {
        mir::client::wayland_runner::quit();
    }
}

void normal_window::handle_keyboard_modifiers(
    wl_keyboard* keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked,
    uint32_t group)
{
    grey_window::handle_keyboard_modifiers(keyboard, serial, mods_depressed, mods_latched, mods_locked, group);

    modifiers = mods_depressed;
}

class satellite : public grey_window
{
public:
    satellite(int32_t width, int32_t height, xdg_positioner* positioner, xdg_toplevel* parent);
    ~satellite();

private:
    zmir_mir_satellite_surface_v1* const mir_surface;

    void handle_repositioned(zmir_mir_satellite_surface_v1* zmir_mir_satellite_surface_v1, uint32_t token);

    static zmir_mir_satellite_surface_v1_listener const satellite_listener;
};

satellite::satellite(int32_t width, int32_t height, xdg_positioner* positioner, xdg_toplevel* parent) :
    grey_window{width, height, 128},
    mir_surface{globals::mir_shell ? zmir_mir_shell_v1_get_satellite_surface(globals::mir_shell, *this, positioner) : nullptr}
{
    xdg_toplevel_set_parent(*this, parent);
    xdg_toplevel_set_title(*this, "satellite");

    if (mir_surface)
    {
        zmir_mir_satellite_surface_v1_add_listener(mir_surface, &satellite_listener, this);
    }
}

satellite::~satellite()
{
    if (mir_surface)
    {
        zmir_mir_satellite_surface_v1_destroy(mir_surface);
    }
}

void satellite::handle_repositioned(zmir_mir_satellite_surface_v1* /*zmir_mir_satellite_surface_v1*/, uint32_t /*token*/)
{
    trace("Received repositioned event");
}

zmir_mir_satellite_surface_v1_listener const satellite::satellite_listener=
{
    .repositioned =  [](void* ctx, auto... args) { static_cast<satellite*>(ctx)->handle_repositioned(args...); },
};

auto make_satellite(normal_window* main_window) -> std::unique_ptr<satellite>
{
    auto const positioner = xdg_wm_base_create_positioner(globals::wm_base);
    auto const [anchor_width, anchor_height] = main_window->size();

    xdg_positioner_set_anchor_rect(positioner, 0, 0, anchor_width, anchor_height);
    xdg_positioner_set_anchor(positioner, satellite_anchor);
    xdg_positioner_set_gravity(positioner, satellite_anchor);
    xdg_positioner_set_offset(positioner, satellite_offset_horizontal, satellite_offset_vertical);
    xdg_positioner_set_constraint_adjustment(positioner, XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_SLIDE_X);

    return std::make_unique<satellite>(100, 400, positioner, *main_window);
}
}

int main(int argc, char* argv[])
{
    parse_options(argc, argv);

    globals::init();

    {
        auto const main_window = std::make_unique<normal_window>(main_width, main_height);
        wl_display_roundtrip(display);

        auto const toolbox = satellite_enabled ? make_satellite(main_window.get()) : std::unique_ptr<satellite>{};
        wl_display_roundtrip(display);

        mir::client::wayland_runner::run(display);
    }

    wl_display_roundtrip(display);

    return 0;
}
