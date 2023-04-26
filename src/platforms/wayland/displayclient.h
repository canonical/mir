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

#ifndef MIR_WAYLAND_DISPLAYCLIENT_H_
#define MIR_WAYLAND_DISPLAYCLIENT_H_

#include <mir/geometry/rectangles.h>
#include <mir/graphics/display.h>
#include <mir/graphics/display_buffer.h>
#include <mir/graphics/display_configuration.h>
#include <mir/renderer/gl/render_target.h>
#include <mir/graphics/gl_config.h>
#include <mir/executor.h>

#include "protocol/xdg-shell-client.h"
#include <wayland-client.h>
#include <EGL/egl.h>

#include <functional>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <mir/geometry/displacement.h>

struct xkb_context;
struct xkb_keymap;
struct xkb_state;

namespace mir
{
namespace graphics
{
namespace wayland
{

class DisplayClient
    : public Executor
{
public:
    DisplayClient(wl_display* display,
    std::shared_ptr<GLConfig> const& gl_config);

    virtual ~DisplayClient();

protected:

    wl_display* const display;

    auto display_configuration() const -> std::unique_ptr<DisplayConfiguration>;
    void for_each_display_sync_group(const std::function<void(DisplaySyncGroup&)>& f);

    void shell_ping(xdg_wm_base* shell, uint32_t serial);

    virtual void keyboard_keymap(wl_keyboard* keyboard, uint32_t format, int32_t fd, uint32_t size);
    virtual void keyboard_enter(wl_keyboard* keyboard, uint32_t serial, wl_surface* surface, wl_array* keys);
    virtual void keyboard_leave(wl_keyboard* keyboard, uint32_t serial, wl_surface* surface);
    virtual void keyboard_key(wl_keyboard* keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state);
    virtual void keyboard_modifiers(
        wl_keyboard* keyboard,
        uint32_t serial,
        uint32_t mods_depressed,
        uint32_t mods_latched,
        uint32_t mods_locked,
        uint32_t group);
    virtual void keyboard_repeat_info(wl_keyboard* wl_keyboard, int32_t rate, int32_t delay);
    xkb_context* keyboard_context() const { return keyboard_context_; }
    xkb_keymap* keyboard_map() const { return keyboard_map_; }
    xkb_state* keyboard_state() const { return keyboard_state_; }

    virtual void pointer_enter(wl_pointer* pointer, uint32_t serial, wl_surface* surface, wl_fixed_t x, wl_fixed_t y);
    virtual void pointer_leave(wl_pointer* pointer, uint32_t serial, wl_surface* surface);
    virtual void pointer_motion(wl_pointer* pointer, uint32_t time, wl_fixed_t x, wl_fixed_t y);
    virtual void pointer_button(wl_pointer* pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state);
    virtual void pointer_axis(wl_pointer* pointer, uint32_t time, uint32_t axis, wl_fixed_t value);
    virtual void pointer_frame(wl_pointer* pointer);
    virtual void pointer_axis_source(wl_pointer* pointer, uint32_t axis_source);
    virtual void pointer_axis_stop(wl_pointer* pointer, uint32_t time, uint32_t axis);
    virtual void pointer_axis_discrete(wl_pointer* pointer, uint32_t axis, int32_t discrete);

    virtual void touch_down(
        wl_touch* touch,
        uint32_t serial,
        uint32_t time,
        wl_surface* surface,
        int32_t id,
        wl_fixed_t x,
        wl_fixed_t y);

    virtual void touch_up(
        wl_touch* touch,
        uint32_t serial,
        uint32_t time,
        int32_t id);

    virtual void touch_motion(
        wl_touch* touch,
        uint32_t time,
        int32_t id,
        wl_fixed_t x,
        wl_fixed_t y);

    virtual void touch_frame(wl_touch* touch);

    virtual void touch_cancel(wl_touch* touch);

    virtual void touch_shape(
        wl_touch* touch,
        int32_t id,
        wl_fixed_t major,
        wl_fixed_t minor);

    virtual void touch_orientation(
        wl_touch* touch,
        int32_t id,
        wl_fixed_t orientation);

    class Output;
    void on_display_config_changed();
    void delete_outputs_to_be_deleted();

    wl_compositor* compositor = nullptr;
    xdg_wm_base* shell = nullptr;
    wl_seat* seat = nullptr;
    wl_shm* shm = nullptr;

    static void new_global(
        void* data,
        struct wl_registry* registry,
        uint32_t id,
        char const* interface,
        uint32_t version);

    static void remove_global(
        void* data,
        struct wl_registry* registry,
        uint32_t name);

    static void add_seat_listener(DisplayClient* self, wl_seat* seat);
    void seat_capabilities(wl_seat* seat, uint32_t capabilities);
    void seat_name(wl_seat* seat, const char* name);

    static void add_shm_listener(DisplayClient* self, wl_shm* shm);
    void shm_format(wl_shm *wl_shm, uint32_t format);
    MirPixelFormat shm_pixel_format{mir_pixel_format_invalid};

    xkb_context* keyboard_context_;
    xkb_keymap* keyboard_map_ = nullptr;
    xkb_state* keyboard_state_ = nullptr;
    bool fake_pointer_frame = false;
    geometry::Displacement pointer_displacement; // Position of current output
    geometry::Displacement touch_displacement;   // Position of current output

    std::unique_ptr<wl_registry, decltype(&wl_registry_destroy)> registry;

    std::mutex mutable outputs_mutex;
    // Outputs that will be dropped during the next Display::configure(). It is unsafe to actually drop them until then.
    std::vector<std::unique_ptr<Output>> outputs_to_be_deleted;
    std::unordered_map<uint32_t, std::unique_ptr<Output>> bound_outputs;
    std::mutex mutable config_change_handlers_mutex;
    std::vector<DisplayConfigurationChangeHandler> config_change_handlers;

    EGLDisplay egldisplay;
    EGLConfig eglconfig;
    EGLContext eglctx;
};
}
}
}

#endif //MIR_WAYLAND_DISPLAYCLIENT_H_
