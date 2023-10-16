/*
 * Copyright © Octopull Ltd.
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

#include "displayclient.h"
#include "wl_egl_display_target.h"
#include "mir/graphics/platform.h"
#include <mir/graphics/pixel_format_utils.h>

#include <wayland-client.h>
#include <wayland-egl.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <xkbcommon/xkbcommon.h>

#include <boost/throw_exception.hpp>

#include <algorithm>
#include <condition_variable>
#include <cstring>
#include <stdlib.h>
#include <stdexcept>

namespace mgw = mir::graphics::wayland;
namespace geom = mir::geometry;

class mgw::DisplayClient::Output  :
    public DisplaySyncGroup,
    public DisplayBuffer
{
public:
    Output(
        wl_output* output,
        DisplayClient* owner);

    ~Output();

    Output(Output const&) = delete;
    Output(Output&&) = delete;
    Output& operator=(Output const&) = delete;
    Output& operator=(Output&&) = delete;

    DisplayConfigurationOutput dcout;
    geom::Size output_size;

    wl_output* const output;
    DisplayClient* const owner_;

    wl_surface* const surface;
    xdg_surface* shell_surface{nullptr};
    xdg_toplevel* shell_toplevel{nullptr};

    EGLContext eglctx{EGL_NO_CONTEXT};
    wl_egl_window* egl_window{nullptr};
    EGLSurface eglsurface{EGL_NO_SURFACE};

    std::optional<geometry::Size> pending_toplevel_size;
    bool has_initialized{false};

    // wl_output events
    void geometry(
        int32_t x,
        int32_t y,
        int32_t physical_width,
        int32_t physical_height,
        int32_t subpixel,
        const char* make,
        const char* model,
        int32_t transform);
    void mode(uint32_t flags, int32_t width, int32_t height, int32_t refresh);
    void scale(int32_t factor);
    void done();

    // XDG shell events
    void toplevel_configure(int32_t width, int32_t height, wl_array* states);
    void surface_configure(uint32_t serial);

    // DisplaySyncGroup implementation
    void for_each_display_buffer(std::function<void(DisplayBuffer&)> const& f) override;
    void post() override;
    std::chrono::milliseconds recommended_sleep() const override;

    // DisplayBuffer implementation
    auto view_area() const -> geometry::Rectangle override;
    bool overlay(std::vector<DisplayElement> const& renderlist) override;
    auto transformation() const -> glm::mat2 override;
    auto target() const -> std::shared_ptr<DisplayTarget> override;
    void set_next_image(std::unique_ptr<Framebuffer> content) override;

private:
    std::unique_ptr<WlDisplayTarget::Framebuffer> next_frame;
    std::shared_ptr<WlDisplayTarget> target_;
};

mgw::DisplayClient::Output::Output(
    wl_output* output,
    DisplayClient* owner) :
    output{output},
    owner_{owner},
    surface{wl_compositor_create_surface(owner->compositor)}
{
    // If building against newer Wayland protocol definitions we may miss trailing fields
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wmissing-field-initializers"
    static wl_output_listener const output_listener{
        [](void* self, auto, auto... args) { static_cast<Output*>(self)->geometry(args...); },
        [](void* self, auto, auto... args) { static_cast<Output*>(self)->mode(args...); },
        [](void* self, auto, auto... args) { static_cast<Output*>(self)->done(args...); },
        [](void* self, auto, auto... args) { static_cast<Output*>(self)->scale(args...); },
    };
    #pragma GCC diagnostic pop
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
    {
        wl_output_destroy(output);
    }

    if (shell_toplevel)
    {
        xdg_toplevel_destroy(shell_toplevel);
    }

    if (shell_surface)
    {
        xdg_surface_destroy(shell_surface);
    }

    wl_surface_destroy(surface);

    if (egl_window != nullptr)
    {
        wl_egl_window_destroy(egl_window);
    }
}

void mgw::DisplayClient::Output::geometry(
    int32_t x,
    int32_t y,
    int32_t physical_width,
    int32_t physical_height,
    int32_t subpixel,
    const char */*make*/,
    const char */*model*/,
    int32_t transform)
{
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

    switch (transform)
    {
    case WL_OUTPUT_TRANSFORM_NORMAL:
        dcout.orientation = mir_orientation_normal;
        break;

    case WL_OUTPUT_TRANSFORM_90:
        dcout.orientation = mir_orientation_left;
        break;

    case WL_OUTPUT_TRANSFORM_180:
        dcout.orientation = mir_orientation_inverted;
        break;

    case WL_OUTPUT_TRANSFORM_270:
        dcout.orientation = mir_orientation_right;
        break;
    }
}

void mgw::DisplayClient::Output::mode(uint32_t flags, int32_t width, int32_t height, int32_t refresh)
{
    auto& modes = dcout.modes;

    auto const mode = DisplayConfigurationMode{{width, height}, refresh/1000.0};
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

void mgw::DisplayClient::Output::scale(int32_t factor)
{
    dcout.scale = factor;
}

void mgw::DisplayClient::Output::done()
{
    if (!shell_surface)
    {
        static xdg_surface_listener const shell_surface_listener{
            [](void* self, auto, auto... args) { static_cast<Output*>(self)->surface_configure(args...); },
        };
        shell_surface = xdg_wm_base_get_xdg_surface(owner_->shell, surface);
        xdg_surface_add_listener(shell_surface, &shell_surface_listener, this);

        static xdg_toplevel_listener const shell_toplevel_listener{
            [](void* self, auto, auto... args) { static_cast<Output*>(self)->toplevel_configure(args...); },
            [](auto...){},
        };
        shell_toplevel = xdg_surface_get_toplevel(shell_surface);
        xdg_toplevel_add_listener(shell_toplevel, &shell_toplevel_listener, this);

        xdg_toplevel_set_fullscreen(shell_toplevel, output);
        wl_surface_set_buffer_scale(surface, round(dcout.scale));
        wl_surface_commit(surface);

        // After the next roundtrip the surface should be configured
    }
    else
    {
        /* TODO: We should handle this by raising a hardware-changed notification and reconfiguring in
         * the subsequent `configure()` call.
         */
    }
}

void mgw::DisplayClient::Output::toplevel_configure(int32_t width, int32_t height, wl_array* states)
{
    (void)states;
    pending_toplevel_size = geometry::Size{
        width ? width : 1280,
        height ? height : 1024};
}

void mgw::DisplayClient::Output::surface_configure(uint32_t serial)
{
    xdg_surface_ack_configure(shell_surface, serial);
    bool const size_is_changed = pending_toplevel_size && (
        !dcout.custom_logical_size || dcout.custom_logical_size.value() != pending_toplevel_size.value());
    dcout.custom_logical_size = pending_toplevel_size.value();
    pending_toplevel_size.reset();
    output_size = dcout.extents().size * dcout.scale;
    if (!has_initialized)
    {
        has_initialized = true;
        target_ = std::make_shared<WlDisplayTarget>(*owner_->target, surface, output_size);
    }
    else if (size_is_changed)
    {
        /* TODO: We should, again, handle this by storing the pending size, raising a hardware-changed
         * notification, and then letting the `configure()` system tear down everything and bring it back
         * up at the new size.
         */
    }
}

void mgw::DisplayClient::Output::for_each_display_buffer(std::function<void(DisplayBuffer & )> const& f)
{
    f(*this);
}

void mgw::DisplayClient::Output::post()
{
    struct FrameSync
    {
        explicit FrameSync(wl_surface* surface):
            surface{surface}
        {
        }

        void init()
        {
            callback = wl_surface_frame(surface);
            static struct wl_callback_listener const frame_listener =
                {
                    [](void* data, auto... args)
                    { static_cast<FrameSync*>(data)->frame_done(args...); },
                };
            wl_callback_add_listener(callback, &frame_listener, this);
        }

        ~FrameSync()
        {
            wl_callback_destroy(callback);
        }

        void frame_done(wl_callback*, uint32_t)
        {
            {
                std::lock_guard lock{mutex};
                posted = true;
            }
            cv.notify_one();
        }

        void wait_for_done()
        {
            std::unique_lock lock{mutex};
            cv.wait_for(lock, std::chrono::milliseconds{100}, [this]{ return posted; });
        }

        wl_surface* const surface;

        wl_callback* callback;
        std::mutex mutex;
        bool posted = false;
        std::condition_variable cv;
    };

    auto const frame_sync = std::make_shared<FrameSync>(surface);
    owner_->spawn([frame_sync]()
                  {
                      frame_sync->init();
                  });

    // Avoid throttling compositing by blocking in eglSwapBuffers().
    // Instead we use the frame "done" notification.
    // TODO: We probably don't need to do this every frame!
    eglSwapInterval(target_->get_egl_display(), 0);

    next_frame->swap_buffers();

    frame_sync->wait_for_done();
}

auto mgw::DisplayClient::Output::recommended_sleep() const -> std::chrono::milliseconds
{
    return std::chrono::milliseconds{0};
}

auto mgw::DisplayClient::Output::view_area() const -> geometry::Rectangle
{
    return dcout.extents();
}

bool mgw::DisplayClient::Output::overlay(std::vector<DisplayElement> const&)
{
    return false;
}

auto mgw::DisplayClient::Output::transformation() const -> glm::mat2
{
    return glm::mat2{1};
}

auto mgw::DisplayClient::Output::target() const -> std::shared_ptr<DisplayTarget>
{
    return target_;
}

namespace
{
template<typename To, typename From>
auto unique_ptr_cast(std::unique_ptr<From> ptr) -> std::unique_ptr<To>
{
    From* unowned_src = ptr.release();
    if (auto to_src = dynamic_cast<To*>(unowned_src))
    {
        return std::unique_ptr<To>{to_src};
    }
    delete unowned_src;
    BOOST_THROW_EXCEPTION((
        std::bad_cast()));
}
}

void mgw::DisplayClient::Output::set_next_image(std::unique_ptr<Framebuffer> content)
{
    if (auto wl_content = unique_ptr_cast<WlDisplayTarget::Framebuffer>(std::move(content)))
    {
        next_frame = std::move(wl_content);
    }
    else
    {
        BOOST_THROW_EXCEPTION((std::logic_error{"set_next_image called with a Framebuffer from a different platform"}));
    }
}

mgw::DisplayClient::DisplayClient(
    wl_display* display,
    std::shared_ptr<WlDisplayTarget> target) :
    display{display},
    target{std::move(target)},
    keyboard_context_{xkb_context_new(XKB_CONTEXT_NO_FLAGS)},
    registry{nullptr, [](auto){}}
{
    if (!keyboard_context_)
    {
        fatal_error("Failed to create XKB context");
    }

    registry = {wl_display_get_registry(display), &wl_registry_destroy};

    static wl_registry_listener const registry_listener = {
        new_global,
        remove_global
    };

    wl_registry_add_listener(registry.get(), &registry_listener, this);

    auto const has_uninitialized_output = [&]()
        {
            for (auto const& pair : bound_outputs)
            {
                if (!pair.second->has_initialized)
                {
                    return true;
                }
            }
            return false;
        };

    // Roundtrip once to bind to all output globals, then keep roundtripping until surfaces for all outputs are
    // initialized
    do
    {
        wl_display_roundtrip(display);
    } while (has_uninitialized_output());
}

void mgw::DisplayClient::on_display_config_changed()
{
    std::unique_lock lock{config_change_handlers_mutex};
    auto const handlers = config_change_handlers;
    lock.unlock();
    for (auto const& handler : handlers)
    {
        handler();
    }
}

void mgw::DisplayClient::delete_outputs_to_be_deleted()
{
    std::lock_guard lock{outputs_mutex};
    outputs_to_be_deleted.clear();
}

mgw::DisplayClient::~DisplayClient()
{
    {
        std::lock_guard lock{outputs_mutex};
        bound_outputs.clear();
    }
    registry.reset();
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
        std::lock_guard lock{self->outputs_mutex};
        self->bound_outputs.insert(
            std::make_pair(
                id,
                std::make_unique<Output>(
                    output,
                    self)));
    }
    else if (strcmp(interface, xdg_wm_base_interface.name) == 0)
    {
        static xdg_wm_base_listener const shell_listener{
            [](void*, xdg_wm_base* shell, uint32_t serial){ xdg_wm_base_pong(shell, serial); },
        };
        self->shell = static_cast<decltype(self->shell)>(
            wl_registry_bind(registry, id, &xdg_wm_base_interface, std::min(version, 1u)));
        xdg_wm_base_add_listener(self->shell, &shell_listener, self);
    }
}

void mgw::DisplayClient::remove_global(
    void* data,
    struct wl_registry* /*registry*/,
    uint32_t id)
{
    DisplayClient* self = static_cast<decltype(self)>(data);

    std::lock_guard lock{self->outputs_mutex};
    auto const output = self->bound_outputs.find(id);
    if (output != self->bound_outputs.end())
    {
        self->outputs_to_be_deleted.push_back(std::move(output->second));
        self->bound_outputs.erase(output);
        self->on_display_config_changed();
    }
    // TODO: We should probably also delete any other globals we've bound to that disappear.
}

void mgw::DisplayClient::keyboard_keymap(wl_keyboard* /*keyboard*/, uint32_t format, int32_t fd, uint32_t size)
{
    if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("platform currently requires a keymap"));
    }

    char* keymap_string = static_cast<decltype(keymap_string)>(mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0));
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
    std::lock_guard lock{outputs_mutex};

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
    std::lock_guard lock{outputs_mutex};

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

// TODO support multiple invocations of seat_capabilities()
// E.g. we should ideally be saving the pointer/keyboard/touch across calls, and release them
// if the new capabilities do not contain the relevant capability.
void mgw::DisplayClient::seat_capabilities(wl_seat* seat, uint32_t capabilities)
{
// If building against newer Wayland protocol definitions we may miss trailing fields
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
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
#ifdef WL_TOUCH_SHAPE_SINCE_VERSION
            [](void* self, auto... args) { static_cast<DisplayClient*>(self)->touch_shape(args...); },
#endif
#ifdef WL_TOUCH_ORIENTATION_SINCE_VERSION
            [](void* self, auto... args) { static_cast<DisplayClient*>(self)->touch_orientation(args...); },
#endif
        };

        wl_touch_add_listener(wl_seat_get_touch(seat), &touch_listener, this);
    }
#pragma GCC diagnostic pop
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
        std::lock_guard lock{outputs_mutex};
        for (auto const& output : bound_outputs)
        {
            outputs.push_back(output.second->dcout);
        }
    }

    return std::make_unique<WaylandDisplayConfiguration>(std::move(outputs));
}

void mgw::DisplayClient::for_each_display_sync_group(const std::function<void(DisplaySyncGroup&)>& f)
{
    std::lock_guard lock{outputs_mutex};
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
    DisplayConfiguration(),
    outputs{rhs.outputs}
{
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
