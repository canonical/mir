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

#include <list>
#include <map>
#include <memory>
#include <string.h>

using namespace std::literals;
namespace wayland_runner = mir::client::wayland_runner;

namespace
{
bool const tracing = getenv("MIR_SHELL_DEMO_TRACE");

int32_t main_width = 400;
int32_t main_height = 400;

mir_positioner_v1_anchor satellite_anchor = MIR_POSITIONER_V1_ANCHOR_LEFT;
int32_t satellite_offset_vertical = 50;
int32_t satellite_offset_horizontal = 0;

void parse_options(int argc, char* argv[])
{
    auto const option_main_width = "width";
    auto const option_main_height = "height";

    struct option const long_options[] = {
        {option_main_width,  required_argument, 0, 0},
        {option_main_height, required_argument, 0, 0},
        {0, 0,                                  0, 0}
    };

    int option_index = 0;

    while_has_no_init_statement:
    if (auto c = getopt_long(argc, argv, "", long_options, &option_index); c != -1)
    {

        switch (c)
        {
        case 0:
            if (option_main_width == long_options[option_index].name)
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
int const no_of_buffers = 2;

auto const display = wl_display_connect(NULL);

class window;
class window_register
{
public:
    void register_window(window* w);
    void deregister_window(window* w);

    auto window_for(wl_surface* surface) -> window*;
    bool is_registered(window* w) const;

private:
    std::map<wl_surface*, window*> windows;
};

namespace globals
{
wl_compositor* compositor;
wl_shm* shm;
wl_seat* seat;
wl_output* output;
xdg_wm_base* wm_base;
mir_shell_v1* mir_shell;

window_register my_windows;

void init();
}

class window
{
public:
    window(int32_t width, int32_t height);
    virtual ~window();

    operator wl_surface*() const { return surface; }

    auto size() const { return std::tuple(width, height); }

    virtual void handle_mouse_button(wl_pointer*, uint32_t serial, uint32_t time, uint32_t button, uint32_t state);
    virtual void handle_keyboard_key(wl_keyboard* keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state);
    virtual void handle_keyboard_modifiers(
        wl_keyboard* keyboard,
        uint32_t serial,
        uint32_t mods_depressed,
        uint32_t mods_latched,
        uint32_t mods_locked,
        uint32_t group);

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
        if (width == width_ && height == height_) return;

        if (width_ > 0) width = width_;
        if (height_ > 0) height = height_;

        if ((width_ > 0) || (height_ > 0))
        {
            redraw();
        }
    }

    void redraw();

private:
    wl_surface* const surface;
    buffer buffers[no_of_buffers];
    int width;
    int height;
    bool need_to_draw = true;
    bool hidden = false;

    void handle_frame_callback(wl_callback* callback, uint32_t);

    void update_free_buffers(wl_buffer* buffer);
    void prepare_buffer(buffer& b);
    auto find_free_buffer() -> buffer*;
    virtual void draw_new_content(buffer* b) = 0;

    static wl_callback_listener const frame_listener;
    static wl_buffer_listener const buffer_listener;

    window(window const&) = delete;
    window& operator=(window const&) = delete;
};

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

    virtual void show_activated() = 0;
    virtual void show_unactivated() = 0;

    static xdg_toplevel_listener const shell_toplevel_listener;
    static xdg_surface_listener const shell_surface_listener;
};

class grey_window : public toplevel
{
public:
    grey_window(int32_t width, int32_t height, unsigned char intensity) :
        toplevel(width, height), intensity(intensity) {}

    grey_window(int32_t width, int32_t height, unsigned char intensity, int title_height) :
        toplevel(width, height), intensity(intensity), title_height{title_height} {}

private:
    unsigned char const intensity;
    unsigned char const alpha = 255;
    int title_height = 20;

    unsigned char intensity_offset = 0;

    void draw_new_content(buffer* b) override;
    virtual void show_activated() override;
    virtual void show_unactivated() override;
};

class regular_window;
class satellite;
class dialog;
class floating_regular;
auto make_satellite(regular_window* main_window) -> std::unique_ptr<satellite>;
auto make_dialog(regular_window* main_window) -> std::unique_ptr<dialog>;
auto make_floater() -> std::unique_ptr<floating_regular>;

class regular_window : public grey_window
{
public:
    regular_window(int32_t width, int32_t height);
    ~regular_window();

    static std::list<std::unique_ptr<satellite>> toolboxs;
    static std::unique_ptr<floating_regular> my_floater;
    static std::unique_ptr<dialog> my_dialog;

protected:
    void
    handle_keyboard_key(wl_keyboard* keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state) override;

    void handle_keyboard_modifiers(
        wl_keyboard* keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked,
        uint32_t group) override;

private:
    mir_regular_surface_v1* const mir_regular_surface;

    uint32_t modifiers = 0;
};

class satellite : public grey_window
{
public:
    void
    handle_keyboard_key(wl_keyboard* keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state) override;

    satellite(int32_t width, int32_t height, mir_positioner_v1* positioner, xdg_toplevel* parent);
    ~satellite();

private:
    mir_satellite_surface_v1* const mir_surface;

    void handle_repositioned(mir_satellite_surface_v1* mir_satellite_surface_v1, uint32_t token);

    static mir_satellite_surface_v1_listener const satellite_listener;
};

class dialog : public grey_window
{
public:
    dialog(int32_t width, int32_t height, regular_window* parent);
    ~dialog();

private:
    mir_dialog_surface_v1* const mir_surface;

    void handle_keyboard_key(wl_keyboard* keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
        override;
};

class floating_regular : public grey_window
{
public:
    floating_regular(int32_t width, int32_t height);
    ~floating_regular();

private:
    mir_floating_regular_surface_v1* const mir_surface;

    void handle_keyboard_key(wl_keyboard* keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
    override;
};

class input_listener
{
public:
    input_listener();
    ~input_listener();

private:
    wl_pointer* const pointer;
    wl_keyboard* const keyboard;

    window* mouse_focus = nullptr;
    window* keyboard_focus = nullptr;

    static wl_pointer_listener const pointer_listener;
    static wl_keyboard_listener const keyboard_listener;

    void handle_mouse_button(wl_pointer*, uint32_t serial, uint32_t time, uint32_t button, uint32_t state);
    void handle_keyboard_key(wl_keyboard* keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state);
    void handle_keyboard_modifiers(
        wl_keyboard* keyboard,
        uint32_t serial,
        uint32_t mods_depressed,
        uint32_t mods_latched,
        uint32_t mods_locked,
        uint32_t group);
    void handle_mouse_enter(wl_pointer*, uint32_t serial, wl_surface*, wl_fixed_t surface_x, wl_fixed_t surface_y);
    void handle_mouse_leave(wl_pointer*, uint32_t serial, wl_surface*);
    void handle_mouse_motion(wl_pointer*, uint32_t time, wl_fixed_t surface_x, wl_fixed_t surface_y);
    void handle_mouse_axis(wl_pointer*, uint32_t time, uint32_t axis, wl_fixed_t value);

    void handle_keyboard_keymap(wl_keyboard* keyboard, uint32_t format, int32_t fd, uint32_t size);
    void handle_keyboard_enter(wl_keyboard* keyboard, uint32_t serial, wl_surface* surface, wl_array* keys);
    void handle_keyboard_leave(wl_keyboard* keyboard, uint32_t serial, wl_surface* surface);
    void handle_keyboard_repeat_info(wl_keyboard* wl_keyboard, int32_t rate, int32_t delay);
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
    else if (strcmp(interface, mir_shell_v1_interface.name) == 0)
    {
        globals::mir_shell = static_cast<mir_shell_v1*>(wl_registry_bind(
            registry, id, &mir_shell_v1_interface, std::min(version, 1u)));
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

void window_register::register_window(window* w)
{
    windows[*w] = w;
}

void window_register::deregister_window(window* w)
{
    windows.erase(*w);
}

bool window_register::is_registered(window* w) const
{
    return w && windows.find(*w) != windows.end();
}

auto window_register::window_for(wl_surface* surface) -> window*
{
    return windows[surface];
}

void window::update_free_buffers(wl_buffer* buffer)
{
    for (int i = 0; i < no_of_buffers; ++i)
    {
        if (buffers[i].buffer == buffer)
        {
            buffers[i].available = true;
        }
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
        redraw();
    }
}

void window::handle_keyboard_key(wl_keyboard*, uint32_t, uint32_t, uint32_t, uint32_t)
{
}

void window::handle_keyboard_modifiers(wl_keyboard*, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)
{
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

    globals::my_windows.register_window(this);
}

window::~window()
{
    globals::my_windows.deregister_window(this);
    for (auto b : buffers)
    {
        wl_buffer_destroy(b.buffer);
    }

    wl_surface_destroy(surface);
}

void window::redraw()
{
    if (auto const b = find_free_buffer())
    {
        draw_new_content(b);

        auto const new_frame_signal = wl_surface_frame(surface);
        wl_callback_add_listener(new_frame_signal, &frame_listener, this);
        wl_surface_attach(surface, b->buffer, 0, 0);
        wl_surface_commit(surface);
        need_to_draw = false;
    }
    else
    {
        need_to_draw = true;
    }
}

void window::handle_mouse_button(wl_pointer*, uint32_t , uint32_t , uint32_t , uint32_t )
{

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
    wl_array* array)
{
    trace("Received toplevel_configure: width %i, height %i", width_, height_);

    bool is_activated = false;
    xdg_toplevel_state* state;
    for (state = static_cast<decltype(state)>((array)->data); (const char*)state < ((const char*)(array)->data + (array)->size); state++)
    {
        if (*state == XDG_TOPLEVEL_STATE_ACTIVATED)
            is_activated = true;
    }

    if (is_activated)
        show_activated();
    else
        show_unactivated();

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

    if (button == BTN_LEFT && state == WL_POINTER_BUTTON_STATE_PRESSED)
        xdg_toplevel_move(xdgtoplevel, globals::seat, serial);

    if (button == BTN_RIGHT && state == WL_POINTER_BUTTON_STATE_PRESSED)
        xdg_toplevel_resize(xdgtoplevel, globals::seat, serial, XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM_RIGHT);
}

void grey_window::draw_new_content(window::buffer* b)
{
    auto const begin = static_cast<uint8_t*>(b->content_area);
    auto const end_bar = begin + b->width * std::min(title_height, b->height) * pixel_size;
    auto const end_all = begin + b->width * b->height * pixel_size;

    memset(begin, std::max(intensity-intensity_offset, 0), end_bar - begin);
    memset(end_bar, intensity, end_all - end_bar);
    for (auto* p = begin; p != end_all; p += pixel_size)
    {
        p[3] = alpha;
    }
}

void grey_window::show_activated()
{
    if (intensity_offset != 32)
    {
        intensity_offset = 32;
        redraw();
    }
}

void grey_window::show_unactivated()
{
    if (intensity_offset != 16)
    {
        intensity_offset = 16;
        redraw();
    }
}

std::list<std::unique_ptr<satellite>> regular_window::toolboxs;
std::unique_ptr<floating_regular> regular_window::my_floater;
std::unique_ptr<dialog> regular_window::my_dialog;

regular_window::regular_window(int32_t width, int32_t height) :
    grey_window(width, height, 192),
    mir_regular_surface{globals::mir_shell? mir_shell_v1_get_regular_surface(globals::mir_shell, *this) : nullptr}
{
    xdg_toplevel_set_title(*this, "normal");
    redraw();
}

regular_window::~regular_window()
{
    if (mir_regular_surface)
    {
        mir_regular_surface_v1_destroy(mir_regular_surface);
    }
}

void regular_window::handle_keyboard_key(wl_keyboard* keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
{
    grey_window::handle_keyboard_key(keyboard, serial, time, key, state);

    if (modifiers == ControlMask && state == WL_KEYBOARD_KEY_STATE_RELEASED)
    {
        switch (key)
        {
        case KEY_Q:wayland_runner::quit();
            break;
        case KEY_T:toolboxs.push_back(make_satellite(this));
            break;
        case KEY_D:my_dialog = make_dialog(this);
            break;
        case KEY_F:my_floater = make_floater();
            break;
        }
    }
    if (state == WL_KEYBOARD_KEY_STATE_RELEASED)
    switch (key)
    {
    case KEY_ESC:
        toolboxs.clear();
        break;
    }
}

void regular_window::handle_keyboard_modifiers(
    wl_keyboard* keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked,
    uint32_t group)
{
    grey_window::handle_keyboard_modifiers(keyboard, serial, mods_depressed, mods_latched, mods_locked, group);

    modifiers = mods_depressed;
}

satellite::satellite(int32_t width, int32_t height, mir_positioner_v1* positioner, xdg_toplevel* parent) :
    grey_window{width, height, 128, 10},
    mir_surface{globals::mir_shell ? mir_shell_v1_get_satellite_surface(globals::mir_shell, *this, positioner) : nullptr}
{
    xdg_toplevel_set_parent(*this, parent);
    xdg_toplevel_set_title(*this, "satellite");

    if (mir_surface)
    {
        mir_satellite_surface_v1_add_listener(mir_surface, &satellite_listener, this);
    }
    redraw();
}

satellite::~satellite()
{
    if (mir_surface)
    {
        mir_satellite_surface_v1_destroy(mir_surface);
    }
}

void satellite::handle_repositioned(mir_satellite_surface_v1* /*mir_satellite_surface_v1*/, uint32_t /*token*/)
{
    trace("Received repositioned event");
}

mir_satellite_surface_v1_listener const satellite::satellite_listener=
{
    .repositioned =  [](void* ctx, auto... args) { static_cast<satellite*>(ctx)->handle_repositioned(args...); },
};

void satellite::handle_keyboard_key(wl_keyboard* keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
{
    grey_window::handle_keyboard_key(keyboard, serial, time, key, state);

    if (state == WL_KEYBOARD_KEY_STATE_RELEASED)
        switch (key)
        {
        case KEY_ESC:
            regular_window::toolboxs.clear();
            break;
        }
}

auto make_satellite(regular_window* main_window) -> std::unique_ptr<satellite>
{
    if (globals::mir_shell)
    {
        auto const positioner = mir_shell_v1_create_positioner(globals::mir_shell);
        auto const [anchor_width, anchor_height] = main_window->size();

        mir_positioner_v1_set_anchor_rect(positioner, 0, 0, anchor_width, anchor_height);
        mir_positioner_v1_set_anchor(positioner, satellite_anchor);
        mir_positioner_v1_set_gravity(positioner, satellite_anchor);
        mir_positioner_v1_set_offset(positioner, satellite_offset_horizontal, satellite_offset_vertical);

        uint32_t constraint_adjustment = MIR_POSITIONER_V1_CONSTRAINT_ADJUSTMENT_SLIDE_X;
        switch (satellite_anchor)
        {
        case MIR_POSITIONER_V1_ANCHOR_LEFT:satellite_anchor = MIR_POSITIONER_V1_ANCHOR_RIGHT;
            break;
        case MIR_POSITIONER_V1_ANCHOR_RIGHT:satellite_anchor = MIR_POSITIONER_V1_ANCHOR_BOTTOM;
            constraint_adjustment = MIR_POSITIONER_V1_CONSTRAINT_ADJUSTMENT_SLIDE_Y;
            break;
        case MIR_POSITIONER_V1_ANCHOR_BOTTOM:satellite_anchor = MIR_POSITIONER_V1_ANCHOR_BOTTOM_LEFT;
            constraint_adjustment = MIR_POSITIONER_V1_CONSTRAINT_ADJUSTMENT_SLIDE_Y;
            break;
        case MIR_POSITIONER_V1_ANCHOR_BOTTOM_LEFT:satellite_anchor = MIR_POSITIONER_V1_ANCHOR_BOTTOM_RIGHT;
            constraint_adjustment =
                MIR_POSITIONER_V1_CONSTRAINT_ADJUSTMENT_SLIDE_Y | MIR_POSITIONER_V1_CONSTRAINT_ADJUSTMENT_SLIDE_X;
            break;
        case MIR_POSITIONER_V1_ANCHOR_BOTTOM_RIGHT:constraint_adjustment =
                                                       MIR_POSITIONER_V1_CONSTRAINT_ADJUSTMENT_SLIDE_Y |
                                                       MIR_POSITIONER_V1_CONSTRAINT_ADJUSTMENT_SLIDE_X;
            [[fallthrough]];
        default:satellite_anchor = MIR_POSITIONER_V1_ANCHOR_LEFT;
            break;
        }
        mir_positioner_v1_set_constraint_adjustment(positioner, constraint_adjustment);

        return std::make_unique<satellite>(100, 200, positioner, *main_window);
    }
    else
    {
        return std::make_unique<satellite>(100, 200, nullptr, *main_window);
    }
}

dialog::dialog(int32_t width, int32_t height, regular_window* parent) :
    grey_window{width, height, 160},
    mir_surface{globals::mir_shell ? mir_shell_v1_get_dialog_surface(globals::mir_shell, *this) : nullptr}
{
    xdg_toplevel_set_parent(*this, *parent);
    xdg_toplevel_set_title(*this, "dialog");
    redraw();
}

dialog::~dialog()
{
    if (mir_surface)
    {
        mir_dialog_surface_v1_destroy(mir_surface);
    }
}

void dialog::handle_keyboard_key(wl_keyboard* keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
{
    grey_window::handle_keyboard_key(keyboard, serial, time, key, state);

    if (state == WL_KEYBOARD_KEY_STATE_RELEASED)
        switch (key)
        {
        case KEY_ESC:
            regular_window::my_dialog.reset();
            break;
        }
}

auto make_dialog(regular_window* main_window) -> std::unique_ptr<dialog>
{
    return std::make_unique<dialog>(200, 200, main_window);
}

floating_regular::floating_regular(int32_t width, int32_t height) :
    grey_window{width, height, 224},
    mir_surface{globals::mir_shell ? mir_shell_v1_get_floating_regular_surface(globals::mir_shell, *this) : nullptr}
{
    redraw();
}

floating_regular::~floating_regular()
{
    if (mir_surface)
    {
        mir_floating_regular_surface_v1_destroy(mir_surface);
    }
}

void floating_regular::handle_keyboard_key(wl_keyboard* keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
{
    grey_window::handle_keyboard_key(keyboard, serial, time, key, state);
    if (state == WL_KEYBOARD_KEY_STATE_RELEASED)
        switch (key)
        {
        case KEY_ESC:
            regular_window::my_floater.reset();
            break;
        }
}

auto make_floater() -> std::unique_ptr<floating_regular>
{
    return std::make_unique<floating_regular>(200, 200);
}

wl_pointer_listener const input_listener::pointer_listener =
    {
        .enter  = [](void* ctx, auto... args) { static_cast<input_listener*>(ctx)->handle_mouse_enter(args...); },
        .leave  = [](void* ctx, auto... args) { static_cast<input_listener*>(ctx)->handle_mouse_leave(args...); },
        .motion = [](void* ctx, auto... args) { static_cast<input_listener*>(ctx)->handle_mouse_motion(args...); },
        .button = [](void* ctx, auto... args) { static_cast<input_listener*>(ctx)->handle_mouse_button(args...); },
        .axis   = [](void* ctx, auto... args) { static_cast<input_listener*>(ctx)->handle_mouse_axis(args...); },
    };

wl_keyboard_listener const input_listener::keyboard_listener =
    {
        [](void* self, auto... args) { static_cast<input_listener*>(self)->handle_keyboard_keymap(args...); },
        [](void* self, auto... args) { static_cast<input_listener*>(self)->handle_keyboard_enter(args...); },
        [](void* self, auto... args) { static_cast<input_listener*>(self)->handle_keyboard_leave(args...); },
        [](void* self, auto... args) { static_cast<input_listener*>(self)->handle_keyboard_key(args...); },
        [](void* self, auto... args) { static_cast<input_listener*>(self)->handle_keyboard_modifiers(args...); },
        [](void* self, auto... args) { static_cast<input_listener*>(self)->handle_keyboard_repeat_info(args...); },
    };
#pragma GCC diagnostic pop

void input_listener::handle_mouse_enter(
    wl_pointer*,
    uint32_t serial,
    wl_surface* surface,
    wl_fixed_t surface_x,
    wl_fixed_t surface_y)
{
    mouse_focus = globals::my_windows.window_for(surface);

    (void)serial;
    trace("Received handle_mouse_enter event: (%f, %f)",
          wl_fixed_to_double(surface_x),
          wl_fixed_to_double(surface_y));
}

void input_listener::handle_mouse_leave(
    wl_pointer*,
    uint32_t serial,
    wl_surface* surface)
{
    if (mouse_focus == globals::my_windows.window_for(surface))
        mouse_focus = nullptr;

    (void)serial;
    trace("Received mouse_exit event\n");
}

void input_listener::handle_mouse_motion(
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

void input_listener::handle_mouse_button(
    wl_pointer* pointer,
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

    if (globals::my_windows.is_registered(mouse_focus))
    {
        mouse_focus->handle_mouse_button(pointer, serial, time, button, state);
    }
}

void input_listener::handle_mouse_axis(
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


void input_listener::handle_keyboard_keymap(wl_keyboard*, uint32_t format, int32_t /*fd*/, uint32_t size)
{
    trace("Received keyboard_keymap: format %i, size %i", format, size);
}

void input_listener::handle_keyboard_enter(wl_keyboard*, uint32_t, wl_surface* surface, wl_array*)
{
    keyboard_focus = globals::my_windows.window_for(surface);

    trace("Received keyboard_enter:\n");
}

void input_listener::handle_keyboard_leave(wl_keyboard*, uint32_t, wl_surface* surface)
{
    if (keyboard_focus == globals::my_windows.window_for(surface))
        keyboard_focus = nullptr;

    trace("Received keyboard_leave:\n");
}

void input_listener::handle_keyboard_repeat_info(wl_keyboard*, int32_t rate, int32_t delay)
{
    trace("Received keyboard_modifiers: rate %i, delay %i", rate, delay);
}

input_listener::input_listener() :
    pointer{wl_seat_get_pointer(globals::seat)},
    keyboard{wl_seat_get_keyboard(globals::seat)}
{
    wl_keyboard_add_listener(keyboard, &keyboard_listener, this);
    wl_pointer_add_listener(pointer, &pointer_listener, this);
}

void
input_listener::handle_keyboard_key(wl_keyboard* keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
{
    trace("Received keyboard_key: key %i, state %i", key, state);
    if (globals::my_windows.is_registered(keyboard_focus))
    {
        keyboard_focus->handle_keyboard_key(keyboard, serial, time, key, state);
    }
}

void input_listener::handle_keyboard_modifiers(
    wl_keyboard* keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked,
    uint32_t group)
{
    trace("Received keyboard_modifiers: depressed %i, latched %i, locked %i, group %i", mods_depressed, mods_latched, mods_locked, group);
    if (globals::my_windows.is_registered(keyboard_focus))
    {
        keyboard_focus->handle_keyboard_modifiers(keyboard, serial, mods_depressed, mods_latched, mods_locked, group);
    }
}

input_listener::~input_listener()
{
    wl_pointer_destroy(pointer);
    wl_keyboard_destroy(keyboard);
}
}

int main(int argc, char* argv[])
{
    parse_options(argc, argv);

    globals::init();

    {
        input_listener input;

        auto const main_window = std::make_unique<regular_window>(main_width, main_height);
        wl_display_roundtrip(display);
        wl_display_roundtrip(display);

        wayland_runner::run(display);
    }

    wl_display_roundtrip(display);

    return 0;
}
