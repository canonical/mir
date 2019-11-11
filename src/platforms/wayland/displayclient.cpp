/*
 * Copyright © 2018-2019 Octopull Ltd.
 * Copyright © 2019 Canonical Ltd.
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

#include "displayclient.h"
#include <mir/graphics/pixel_format_utils.h>

#include <wayland-client.h>
#include <wayland-egl.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <xkbcommon/xkbcommon.h>

#include <boost/throw_exception.hpp>

#include <cstring>
#include <stdlib.h>
#include <system_error>
#include <algorithm>

namespace mgw = mir::graphics::wayland;

class mgw::DisplayClient::Output  :
    public DisplaySyncGroup,
    public renderer::gl::RenderTarget,
    public NativeDisplayBuffer,
    public DisplayBuffer
{
public:
    Output(
        wl_output* output,
        DisplayClient* owner,
        std::function<void(Output const&)> on_constructed,
        std::function<void(Output const&)> on_change);

    ~Output();

    Output(Output const&) = delete;
    Output(Output&&) = delete;
    Output& operator=(Output const&) = delete;
    Output& operator=(Output&&) = delete;

    DisplayConfigurationOutput dcout;

    static void done(void* data, wl_output* output);

    static void geometry(
        void* data,
        wl_output* wl_output,
        int32_t x,
        int32_t y,
        int32_t physical_width,
        int32_t physical_height,
        int32_t subpixel,
        const char* make,
        const char* model,
        int32_t transform);

    static void mode(
        void* data,
        wl_output* wl_output,
        uint32_t flags,
        int32_t width,
        int32_t height,
        int32_t refresh);

    static void scale(void* data, wl_output* wl_output, int32_t factor);

    static wl_output_listener const output_listener;

    wl_output* const output;
    DisplayClient const* const owner;

    wl_surface* const surface;
    wl_shell_surface* window{nullptr};

    EGLContext eglctx{EGL_NO_CONTEXT};
    EGLSurface eglsurface{EGL_NO_SURFACE};

    std::function<void(Output const&)> on_done;

    // DisplaySyncGroup implementation
    void for_each_display_buffer(std::function<void(DisplayBuffer&)> const& /*f*/) override;
    void post() override;
    std::chrono::milliseconds recommended_sleep() const override;

    // DisplayBuffer implementation
    auto view_area() const -> geometry::Rectangle override;
    bool overlay(RenderableList const& renderlist) override;
    auto transformation() const -> glm::mat2 override;
    auto native_display_buffer() -> NativeDisplayBuffer* override;

    // RenderTarget implementation
    void make_current() override;
    void release_current() override;
    void swap_buffers() override;
    void bind() override;
};

namespace
{
static EGLint const ctxattribs[] =
    {
#if MIR_SERVER_EGL_OPENGL_BIT == EGL_OPENGL_ES2_BIT
        EGL_CONTEXT_CLIENT_VERSION, 2,
#endif
        EGL_NONE
    };
}

void mgw::DisplayClient::Output::geometry(
    void* data,
    struct wl_output* /*wl_output*/,
    int32_t x,
    int32_t y,
    int32_t physical_width,
    int32_t physical_height,
    int32_t subpixel,
    const char */*make*/,
    const char */*model*/,
    int32_t /*transform*/)
{
    auto output = static_cast<Output*>(data);

    auto& dcout = output->dcout;
    dcout.top_left = {x, y};
    dcout.physical_size_mm = {physical_width, physical_height};

    switch (subpixel)
    {
    case WL_OUTPUT_SUBPIXEL_UNKNOWN:
        dcout.subpixel_arrangement = MirSubpixelArrangement::mir_subpixel_arrangement_unknown;
        break;

    case WL_OUTPUT_SUBPIXEL_NONE:
        dcout.subpixel_arrangement = MirSubpixelArrangement::mir_subpixel_arrangement_none;
        break;

    case WL_OUTPUT_SUBPIXEL_HORIZONTAL_RGB:
        dcout.subpixel_arrangement = MirSubpixelArrangement::mir_subpixel_arrangement_horizontal_rgb;
        break;

    case WL_OUTPUT_SUBPIXEL_HORIZONTAL_BGR:
        dcout.subpixel_arrangement = MirSubpixelArrangement::mir_subpixel_arrangement_horizontal_bgr;
        break;

    case WL_OUTPUT_SUBPIXEL_VERTICAL_RGB:
        dcout.subpixel_arrangement = MirSubpixelArrangement::mir_subpixel_arrangement_vertical_rgb;
        break;

    case WL_OUTPUT_SUBPIXEL_VERTICAL_BGR:
        dcout.subpixel_arrangement = MirSubpixelArrangement::mir_subpixel_arrangement_vertical_bgr;
        break;

    default:
        dcout.subpixel_arrangement = MirSubpixelArrangement::mir_subpixel_arrangement_unknown;
        break;
    }
}


void mgw::DisplayClient::Output::mode(
    void *data,
    struct wl_output* /*wl_output*/,
    uint32_t flags,
    int32_t width,
    int32_t height,
    int32_t refresh)
{
    auto const output = static_cast<Output*>(data);
    auto& dcout = output->dcout;
    auto& modes = dcout.modes;

    auto const mode = DisplayConfigurationMode{{width, height}, refresh/100000.0};
    bool const is_current   = flags & WL_OUTPUT_MODE_CURRENT;
    bool const is_preferred = flags & WL_OUTPUT_MODE_PREFERRED;

    auto const found = std::find_if(begin(modes), end(modes), [&](auto const& m)
        { return mode.size == m.size && mode.vrefresh_hz == m.vrefresh_hz; });

    if (found != end(modes))
    {
        if (is_current) dcout.current_mode_index = distance(begin(modes), found);
        if (is_preferred) dcout.preferred_mode_index = distance(begin(modes), found);
    }
    else
    {
        modes.push_back(mode);
        if (is_current) dcout.current_mode_index = modes.size() - 1;
        if (is_preferred) dcout.preferred_mode_index = modes.size() - 1;
    }
}

void mgw::DisplayClient::Output::scale(void* /*data*/, wl_output* /*wl_output*/, int32_t /*factor*/)
{
}

mgw::DisplayClient::Output::Output(
    wl_output* output,
    DisplayClient* owner,
    std::function<void(Output const&)> on_constructed,
    std::function<void(Output const&)> on_change) :
    output{output},
    owner{owner},
    surface{wl_compositor_create_surface(owner->compositor)},
    on_done{[this, on_constructed = std::move(on_constructed), on_change=std::move(on_change)]
        (Output const& o) mutable { on_constructed(o), on_done = std::move(on_change); }}
{
    wl_output_add_listener(output, &output_listener, this);

    dcout.id = (DisplayConfigurationOutputId)owner->bound_outputs.size();
    dcout.card_id = DisplayConfigurationCardId{1};
    dcout.type = DisplayConfigurationOutputType::unknown;
    dcout.pixel_formats = {mir_pixel_format_argb_8888,mir_pixel_format_xrgb_8888};
    dcout.current_format = mir_pixel_format_xrgb_8888;
    dcout.connected = true;
    dcout.used = true;
    dcout.power_mode = mir_power_mode_on;
    dcout.orientation = MirOrientation::mir_orientation_normal;
    dcout.scale = 1.0;
    dcout.form_factor = MirFormFactor::mir_form_factor_monitor;
    dcout.gamma_supported = MirOutputGammaSupported::mir_output_gamma_unsupported;
}

mgw::DisplayClient::Output::~Output()
{
    if (output)
        wl_output_destroy(output);

    if (window)
        wl_shell_surface_destroy(window);

    wl_surface_destroy(surface);

    if (eglsurface != EGL_NO_SURFACE)
        eglDestroySurface(owner->egldisplay, eglsurface);

    if (eglctx != EGL_NO_CONTEXT)
        eglDestroyContext(owner->egldisplay, eglctx);
}

wl_output_listener const mgw::DisplayClient::Output::output_listener = {
    &geometry,
    &mode,
    &done,
    &scale,
};

void mgw::DisplayClient::Output::done(void* data, struct wl_output* /*wl_output*/)
{
    auto output = static_cast<Output*>(data);
    output->on_done(*output);
}

void mgw::DisplayClient::Output::for_each_display_buffer(std::function<void(DisplayBuffer & )> const& f)
{
    if (!window)
    {
        static wl_shell_surface_listener const shell_surface_listener{
            [](void*, wl_shell_surface* shell_surface, uint32_t serial){ wl_shell_surface_pong(shell_surface, serial); },
            [](void*, wl_shell_surface*, uint32_t, int32_t, int32_t){},
            [](void*, wl_shell_surface*){}
        };

        window = wl_shell_get_shell_surface(owner->shell, surface);
        wl_shell_surface_add_listener(window, &shell_surface_listener, this);
        wl_shell_surface_set_fullscreen(window, WL_SHELL_SURFACE_FULLSCREEN_METHOD_SCALE, 0, output);
        wl_display_dispatch(owner->display);

        auto const& size = dcout.modes[dcout.current_mode_index].size;

        eglsurface = eglCreatePlatformWindowSurface(
            owner->egldisplay,
            owner->eglconfig,
            wl_egl_window_create(surface, size.width.as_int(), size.height.as_int()), nullptr);
    }

    f(*this);
}

void mgw::DisplayClient::Output::post()
{
}

auto mgw::DisplayClient::Output::recommended_sleep() const -> std::chrono::milliseconds
{
    return std::chrono::milliseconds{0};
}

auto mgw::DisplayClient::Output::view_area() const -> geometry::Rectangle
{
    return dcout.extents();
}

bool mgw::DisplayClient::Output::overlay(mir::graphics::RenderableList const&)
{
    return false;
}

auto mgw::DisplayClient::Output::transformation() const -> glm::mat2
{
    return glm::mat2{1};
}

auto mgw::DisplayClient::Output::native_display_buffer() -> NativeDisplayBuffer*
{
    return this;
}

void mgw::DisplayClient::Output::make_current()
{
    if (eglctx == EGL_NO_CONTEXT)
        eglctx = eglCreateContext(owner->egldisplay, owner->eglconfig, owner->eglctx, ctxattribs);

    if (!eglMakeCurrent(owner->egldisplay, eglsurface, eglsurface, eglctx))
        throw std::runtime_error("Can't eglMakeCurrent");
}

void mgw::DisplayClient::Output::release_current()
{
    eglMakeCurrent(owner->egldisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

void mgw::DisplayClient::Output::swap_buffers()
{
    if (eglSwapBuffers(owner->egldisplay, eglsurface) != EGL_TRUE)
        fatal_error("Failed to perform buffer swap");
}

void mgw::DisplayClient::Output::bind()
{
}

mgw::DisplayClient::DisplayClient(
    wl_display* display,
    std::shared_ptr<GLConfig> const& gl_config) :
    display{display},
    keyboard_context_{xkb_context_new(XKB_CONTEXT_NO_FLAGS)},
    registry{nullptr, [](auto){}}
{
    registry = {wl_display_get_registry(display), &wl_registry_destroy};

    static wl_registry_listener const registry_listener = {
        new_global,
        remove_global
    };

    wl_registry_add_listener(registry.get(), &registry_listener, this);

    auto format = shm_pixel_format;

    EGLint const cfgattribs[] =
        {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_RED_SIZE, red_channel_depth(format),
            EGL_GREEN_SIZE, green_channel_depth(format),
            EGL_BLUE_SIZE, blue_channel_depth(format),
            EGL_ALPHA_SIZE, alpha_channel_depth(format),
            EGL_DEPTH_SIZE, gl_config->depth_buffer_bits(),
            EGL_STENCIL_SIZE, gl_config->stencil_buffer_bits(),
            EGL_RENDERABLE_TYPE, MIR_SERVER_EGL_OPENGL_BIT,
            EGL_NONE
        };

    eglBindAPI(MIR_SERVER_EGL_OPENGL_API);

    egldisplay = eglGetDisplay((EGLNativeDisplayType)(display));
    if (egldisplay == EGL_NO_DISPLAY)
        throw std::runtime_error("Can't eglGetDisplay");

    EGLint major;
    EGLint minor;
    if (!eglInitialize(egldisplay, &major, &minor))
        throw std::runtime_error("Can't eglInitialize");

    if (major != 1 || minor < 4)
        throw std::runtime_error("EGL version is not at least 1.4");

    EGLint neglconfigs;
    if (!eglChooseConfig(egldisplay, cfgattribs, &eglconfig, 1, &neglconfigs))
        throw std::runtime_error("Could not eglChooseConfig");

    if (neglconfigs == 0)
        throw std::runtime_error("No EGL config available");

    eglctx = eglCreateContext(egldisplay, eglconfig, EGL_NO_CONTEXT, ctxattribs);
    if (eglctx == EGL_NO_CONTEXT)
        throw std::runtime_error("eglCreateContext failed");

    wl_display_roundtrip(display);
}

void mgw::DisplayClient::on_output_changed(Output const* /*output*/)
{
}

void mgw::DisplayClient::on_output_gone(Output const* /*output*/)
{
}

void mgw::DisplayClient::on_new_output(Output const* /*output*/)
{
}

mgw::DisplayClient::~DisplayClient()
{
    eglMakeCurrent(egldisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

    {
        std::lock_guard<decltype(outputs_mutex)> lock{outputs_mutex};
        bound_outputs.clear();
    }
    registry.reset();
    wl_display_roundtrip(display);

    eglDestroyContext(egldisplay, eglctx);
    eglTerminate(egldisplay);
}

void mgw::DisplayClient::new_global(
    void* data,
    struct wl_registry* registry,
    uint32_t id,
    char const* interface,
    uint32_t version)
{
    (void)version;
    DisplayClient* self = static_cast<decltype(self)>(data);

    if (strcmp(interface, "wl_compositor") == 0)
    {
        self->compositor =
            static_cast<decltype(self->compositor)>(wl_registry_bind(registry, id, &wl_compositor_interface, std::min(version, 3u)));
    }
    else if (strcmp(interface, "wl_shm") == 0)
    {
        self->shm = static_cast<decltype(self->shm)>(wl_registry_bind(registry, id, &wl_shm_interface, std::min(version, 1u)));
        // Normally we'd add a listener to pick up the supported formats here
        // As luck would have it, I know that argb8888 is the only format we support :)
        // {arg} TODO needs fixing
        add_shm_listener(self, self->shm);
    }
    else if (strcmp(interface, "wl_seat") == 0)
    {
        if (version < 5) self->fake_pointer_frame = true;
        self->seat = static_cast<decltype(self->seat)>(wl_registry_bind(registry, id, &wl_seat_interface, std::min(version, 6u)));
        add_seat_listener(self, self->seat);
    }
    else if (strcmp(interface, "wl_output") == 0)
    {
        auto output =
            static_cast<wl_output*>(wl_registry_bind(registry, id, &wl_output_interface, std::min(version, 2u)));
        std::lock_guard<decltype(self->outputs_mutex)> lock{self->outputs_mutex};
        self->bound_outputs.insert(
            std::make_pair(
                id,
                std::make_unique<Output>(
                    output,
                    self,
                    [self](Output const& output) { self->on_new_output(&output); },
                    [self](Output const& output) { self->on_output_changed(&output); })));
    }
    else if (strcmp(interface, "wl_shell") == 0)
    {
        self->shell = static_cast<decltype(self->shell)>(wl_registry_bind(registry, id, &wl_shell_interface, std::min(version, 1u)));
    }
}

void mgw::DisplayClient::remove_global(
    void* data,
    struct wl_registry* /*registry*/,
    uint32_t id)
{
    DisplayClient* self = static_cast<decltype(self)>(data);

    std::lock_guard<decltype(self->outputs_mutex)> lock{self->outputs_mutex};
    auto const output = self->bound_outputs.find(id);
    if (output != self->bound_outputs.end())
    {
        self->on_output_gone(output->second.get());
        self->bound_outputs.erase(output);
    }
    // TODO: We should probably also delete any other globals we've bound to that disappear.
}

void mgw::DisplayClient::keyboard_keymap(wl_keyboard* /*keyboard*/, uint32_t /*format*/, int32_t fd, uint32_t size)
{
    char* keymap_string = static_cast<decltype(keymap_string)>(mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0));
    xkb_keymap_unref(keyboard_map_);
    keyboard_map_ = xkb_keymap_new_from_string(keyboard_context(), keymap_string, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
    munmap(keymap_string, size);
    close (fd);
    xkb_state_unref(keyboard_state_);
    keyboard_state_ = xkb_state_new(keyboard_map_);
}

void mgw::DisplayClient::keyboard_enter(
    wl_keyboard* /*keyboard*/,
    uint32_t /*serial*/,
    wl_surface* /*surface*/,
    wl_array* /*keys*/)
{
}

void mgw::DisplayClient::keyboard_leave(wl_keyboard* /*keyboard*/, uint32_t /*serial*/, wl_surface* /*surface*/)
{
}

void mgw::DisplayClient::keyboard_key(
    wl_keyboard* /*keyboard*/,
    uint32_t /*serial*/,
    uint32_t /*time*/,
    uint32_t /*key*/,
    uint32_t /*state*/)
{
}

void mgw::DisplayClient::keyboard_modifiers(
    wl_keyboard */*keyboard*/,
    uint32_t /*serial*/, uint32_t mods_depressed,
    uint32_t mods_latched,
    uint32_t mods_locked,
    uint32_t group)
{
    if (keyboard_state_)
        xkb_state_update_mask(keyboard_state_, mods_depressed, mods_latched, mods_locked, 0, 0, group);
}

void mgw::DisplayClient::keyboard_repeat_info(wl_keyboard* /*wl_keyboard*/, int32_t /*rate*/, int32_t /*delay*/)
{
}

void mgw::DisplayClient::pointer_enter(
    wl_pointer* /*pointer*/,
    uint32_t /*serial*/,
    wl_surface* surface,
    wl_fixed_t /*x*/,
    wl_fixed_t /*y*/)
{
    std::lock_guard<decltype(outputs_mutex)> lock{outputs_mutex};

    for (auto const& out : bound_outputs)
    {
        if (surface == out.second->surface)
        {
            pointer_displacement = out.second->dcout.top_left - geometry::Point{};
            break;
        }
    }
}

void mgw::DisplayClient::pointer_leave(wl_pointer* /*pointer*/, uint32_t /*serial*/, wl_surface* /*surface*/)
{
}

void mgw::DisplayClient::pointer_motion(wl_pointer* pointer, uint32_t /*time*/, wl_fixed_t /*x*/, wl_fixed_t /*y*/)
{
    if (fake_pointer_frame)
        pointer_frame(pointer);
}

void mgw::DisplayClient::pointer_button(
    wl_pointer* pointer,
    uint32_t /*serial*/,
    uint32_t /*time*/,
    uint32_t /*button*/,
    uint32_t /*state*/)
{
    if (fake_pointer_frame)
        pointer_frame(pointer);
}

void mgw::DisplayClient::pointer_axis(
    wl_pointer* pointer,
    uint32_t /*time*/,
    uint32_t /*axis*/,
    wl_fixed_t /*value*/)
{
    if (fake_pointer_frame)
        pointer_frame(pointer);
}

void mgw::DisplayClient::pointer_frame(wl_pointer* /*pointer*/)
{
}

void mgw::DisplayClient::pointer_axis_source(wl_pointer* /*pointer*/, uint32_t /*axis_source*/)
{
}

void mgw::DisplayClient::pointer_axis_stop(wl_pointer* /*pointer*/, uint32_t /*time*/, uint32_t /*axis*/)
{
}

void mgw::DisplayClient::pointer_axis_discrete(wl_pointer* /*pointer*/, uint32_t /*axis*/, int32_t /*discrete*/)
{
}

void mgw::DisplayClient::touch_down(
    wl_touch* /*touch*/,
    uint32_t /*serial*/,
    uint32_t /*time*/,
    wl_surface* surface,
    int32_t /*id*/,
    wl_fixed_t /*x*/,
    wl_fixed_t /*y*/)
{
    std::lock_guard<decltype(outputs_mutex)> lock{outputs_mutex};

    for (auto const& out : bound_outputs)
    {
        if (surface == out.second->surface)
        {
            touch_displacement = out.second->dcout.top_left - geometry::Point{};
            break;
        }
    }
}

void mgw::DisplayClient::touch_up(
    wl_touch* /*touch*/,
    uint32_t /*serial*/,
    uint32_t /*time*/,
    int32_t /*id*/)
{
}

void mgw::DisplayClient::touch_motion(
    wl_touch* /*touch*/,
    uint32_t /*time*/,
    int32_t /*id*/,
    wl_fixed_t /*x*/,
    wl_fixed_t /*y*/)
{
}

void mgw::DisplayClient::touch_frame(wl_touch* /*touch*/)
{
}
    
void mgw::DisplayClient::touch_cancel(wl_touch* /*touch*/)
{
}

void mgw::DisplayClient::touch_shape(
    wl_touch* /*touch*/,
    int32_t /*id*/,
    wl_fixed_t /*major*/,
    wl_fixed_t /*minor*/)
{
}

void mgw::DisplayClient::touch_orientation(
    wl_touch* /*touch*/,
    int32_t /*id*/,
    wl_fixed_t /*orientation*/)
{
}

void mgw::DisplayClient::seat_capabilities(wl_seat* seat, uint32_t capabilities)
{
    if (capabilities & WL_SEAT_CAPABILITY_POINTER) {
        static wl_pointer_listener pointer_listener =
            {
                [](void* self, auto... args) { static_cast<DisplayClient*>(self)->pointer_enter(args...); },
                [](void* self, auto... args) { static_cast<DisplayClient*>(self)->pointer_leave(args...); },
                [](void* self, auto... args) { static_cast<DisplayClient*>(self)->pointer_motion(args...); },
                [](void* self, auto... args) { static_cast<DisplayClient*>(self)->pointer_button(args...); },
                [](void* self, auto... args) { static_cast<DisplayClient*>(self)->pointer_axis(args...); },
                [](void* self, auto... args) { static_cast<DisplayClient*>(self)->pointer_frame(args...); },
                [](void* self, auto... args) { static_cast<DisplayClient*>(self)->pointer_axis_source(args...); },
                [](void* self, auto... args) { static_cast<DisplayClient*>(self)->pointer_axis_stop(args...); },
                [](void* self, auto... args) { static_cast<DisplayClient*>(self)->pointer_axis_discrete(args...); },
            };

        struct wl_pointer *pointer = wl_seat_get_pointer(seat);
        wl_pointer_add_listener (pointer, &pointer_listener, this);
    }

    if (capabilities & WL_SEAT_CAPABILITY_KEYBOARD)
    {
        static struct wl_keyboard_listener keyboard_listener =
            {
                [](void* self, auto... args) { static_cast<DisplayClient*>(self)->keyboard_keymap(args...); },
                [](void* self, auto... args) { static_cast<DisplayClient*>(self)->keyboard_enter(args...); },
                [](void* self, auto... args) { static_cast<DisplayClient*>(self)->keyboard_leave(args...); },
                [](void* self, auto... args) { static_cast<DisplayClient*>(self)->keyboard_key(args...); },
                [](void* self, auto... args) { static_cast<DisplayClient*>(self)->keyboard_modifiers(args...); },
                [](void* self, auto... args) { static_cast<DisplayClient*>(self)->keyboard_repeat_info(args...); },
            };

        wl_keyboard_add_listener(wl_seat_get_keyboard(seat), &keyboard_listener, this);
    }

    if (capabilities & WL_SEAT_CAPABILITY_TOUCH)
    {
        static struct wl_touch_listener touch_listener =
        {
            [](void* self, auto... args) { static_cast<DisplayClient*>(self)->touch_down(args...); },
            [](void* self, auto... args) { static_cast<DisplayClient*>(self)->touch_up(args...); },
            [](void* self, auto... args) { static_cast<DisplayClient*>(self)->touch_motion(args...); },
            [](void* self, auto... args) { static_cast<DisplayClient*>(self)->touch_frame(args...); },
            [](void* self, auto... args) { static_cast<DisplayClient*>(self)->touch_cancel(args...); },
            [](void* self, auto... args) { static_cast<DisplayClient*>(self)->touch_shape(args...); },
            [](void* self, auto... args) { static_cast<DisplayClient*>(self)->touch_orientation(args...); },
        };

        wl_touch_add_listener(wl_seat_get_touch(seat), &touch_listener, this);
    }
}

void mgw::DisplayClient::seat_name(wl_seat* /*seat*/, const char */*name*/)
{
}

void mgw::DisplayClient::add_seat_listener(DisplayClient* self, wl_seat* seat)
{
    static struct wl_seat_listener seat_listener =
        {
            [](void* self, auto... args) { static_cast<DisplayClient*>(self)->seat_capabilities(args...); },
            [](void* self, auto... args) { static_cast<DisplayClient*>(self)->seat_name(args...); },
        };

    wl_seat_add_listener(seat, &seat_listener, self);
}

void mgw::DisplayClient::add_shm_listener(DisplayClient* self, wl_shm* shm)
{
    static struct wl_shm_listener shm_listener =
        {
            [](void* self, auto... args) { static_cast<DisplayClient*>(self)->shm_format(args...); },
        };

    wl_shm_add_listener(shm, &shm_listener, self);
}

void mir::graphics::wayland::DisplayClient::shm_format(wl_shm* /*wl_shm*/, uint32_t format)
{
    switch (format)
    {
    case WL_SHM_FORMAT_ARGB8888:
        if (shm_pixel_format != mir_pixel_format_xrgb_8888)
            shm_pixel_format = mir_pixel_format_argb_8888;
        break;

    case WL_SHM_FORMAT_XRGB8888:
        shm_pixel_format = mir_pixel_format_xrgb_8888;
    }
}

namespace mir
{
namespace graphics
{
namespace wayland
{
namespace
{
class WaylandDisplayConfiguration : public DisplayConfiguration
{
public:
    explicit WaylandDisplayConfiguration(std::vector<DisplayConfigurationOutput> && outputs);
    WaylandDisplayConfiguration(WaylandDisplayConfiguration const&);

    void for_each_card(std::function<void(DisplayConfigurationCard const&)> f) const override;
    void for_each_output(std::function<void(DisplayConfigurationOutput const&)> f) const override;
    void for_each_output(std::function<void(UserDisplayConfigurationOutput&)> f) override;
    auto clone() const -> std::unique_ptr<DisplayConfiguration> override;

private:
    DisplayConfigurationCard card{ DisplayConfigurationCardId{1}, 9 };
    std::vector<DisplayConfigurationOutput> outputs;
};
}
}
}
}

auto mgw::DisplayClient::display_configuration() const -> std::unique_ptr<DisplayConfiguration>
{
    std::vector<DisplayConfigurationOutput> outputs;

    {
        std::lock_guard<decltype(outputs_mutex)> lock{outputs_mutex};
        for (auto const& output : bound_outputs)
        {
            outputs.push_back(output.second->dcout);
        }
    }

    return std::make_unique<WaylandDisplayConfiguration>(std::move(outputs));
}

void mgw::DisplayClient::for_each_display_sync_group(const std::function<void(DisplaySyncGroup&)>& f)
{
    std::lock_guard<decltype(outputs_mutex)> lock{outputs_mutex};
    for (auto& output : bound_outputs)
    {
        f(*output.second);
    }
}

mgw::WaylandDisplayConfiguration::WaylandDisplayConfiguration(std::vector<DisplayConfigurationOutput> && outputs) :
    outputs{outputs}
{
}

mgw::WaylandDisplayConfiguration::WaylandDisplayConfiguration(WaylandDisplayConfiguration const& rhs) :
    DisplayConfiguration{},
    outputs{rhs.outputs}
{
}

void mgw::WaylandDisplayConfiguration::for_each_card(
    std::function<void(DisplayConfigurationCard const&)> f) const
{
    f(card);
}

void mgw::WaylandDisplayConfiguration::for_each_output(
    std::function<void(DisplayConfigurationOutput const&)> f) const
{
    for (auto const& o: outputs)
    {
        f(o);
    }
}

void mgw::WaylandDisplayConfiguration::for_each_output(
    std::function<void(UserDisplayConfigurationOutput & )> f)
{
    for (auto& o: outputs)
    {
        UserDisplayConfigurationOutput udo{o};
        f(udo);
    }
}

auto mgw::WaylandDisplayConfiguration::clone() const -> std::unique_ptr<DisplayConfiguration>
{
    return std::make_unique<WaylandDisplayConfiguration>(*this);
}
