/*
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

#ifndef MIR_PLATFORMS_WAYLAND_DISPLAY_H_
#define MIR_PLATFORMS_WAYLAND_DISPLAY_H_

#include "displayclient.h"

#include <mir/fd.h>
#include <mir/events/contact_state.h>
#include <mir/geometry/displacement.h>
#include <mir/graphics/display.h>
#include <mir/graphics/display_report.h>
#include <mir/renderer/gl/context_source.h>

#include <xkbcommon/xkbcommon.h>

#include <thread>

namespace mir
{
namespace platform
{
namespace wayland
{
class Cursor;
}
}
namespace input
{
namespace wayland
{
class KeyboardInput;
class PointerInput;
class TouchInput;
}
}
namespace graphics
{
namespace wayland
{
class Display : public mir::graphics::Display, public mir::graphics::NativeDisplay,
                public mir::renderer::gl::ContextSource, DisplayClient
{
public:
    Display(
        wl_display* const wl_display,
        std::shared_ptr<GLConfig> const& gl_config,
        std::shared_ptr<DisplayReport> const& report);

    ~Display();

    // Set the sinks for input devices. (May be null if there's no corresponding device.)
    static void set_keyboard_sink(std::shared_ptr<input::wayland::KeyboardInput> const& keyboard_sink);
    static void set_pointer_sink(std::shared_ptr<input::wayland::PointerInput> const& pointer_sink);
    static void set_touch_sink(std::shared_ptr<input::wayland::TouchInput> const& touch_sink);

    void for_each_display_sync_group(const std::function<void(DisplaySyncGroup&)>& f) override;

    auto configuration() const -> std::unique_ptr<DisplayConfiguration> override;

    bool apply_if_configuration_preserves_display_buffers(DisplayConfiguration const& conf) override;

    void configure(DisplayConfiguration const& conf) override;

    void register_configuration_change_handler(EventHandlerRegister& handlers,
        DisplayConfigurationChangeHandler const& conf_change_handler) override;

    void register_pause_resume_handlers(EventHandlerRegister& handlers,
        DisplayPauseHandler const& pause_handler, DisplayResumeHandler const& resume_handler) override;

    void pause() override;

    void resume() override;

    auto create_hardware_cursor() -> std::shared_ptr<Cursor>override;

    auto create_virtual_output(int width, int height) -> std::unique_ptr<VirtualOutput> override;

    NativeDisplay* native_display() override;

    auto last_frame_on(unsigned output_id) const -> Frame override;

    auto create_gl_context() const -> std::unique_ptr<mir::renderer::gl::Context> override;

private:

    void keyboard_key(wl_keyboard* keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state) override;

    void pointer_motion(wl_pointer* pointer, uint32_t time, wl_fixed_t x, wl_fixed_t y) override;

    void pointer_button(wl_pointer* pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state) override;

    void pointer_axis(wl_pointer* pointer, uint32_t time, uint32_t axis, wl_fixed_t value) override;

    void pointer_frame(wl_pointer* pointer) override;

    void pointer_enter(wl_pointer* pointer, uint32_t serial, wl_surface* surface, wl_fixed_t x, wl_fixed_t y) override;

    void pointer_leave(wl_pointer* pointer, uint32_t serial, wl_surface* surface) override;

    void touch_down(
        wl_touch* touch, uint32_t serial, uint32_t time, wl_surface* surface, int32_t id, wl_fixed_t x,
        wl_fixed_t y) override;

    void touch_up(wl_touch* touch, uint32_t serial, uint32_t time, int32_t id) override;

    void touch_motion(wl_touch* touch, uint32_t time, int32_t id, wl_fixed_t x, wl_fixed_t y) override;

    void touch_frame(wl_touch* touch) override;

    void touch_cancel(wl_touch* touch) override;

    void touch_shape(wl_touch* touch, int32_t id, wl_fixed_t major, wl_fixed_t minor) override;

    void touch_orientation(wl_touch* touch, int32_t id, wl_fixed_t orientation) override;

private:
    std::shared_ptr<DisplayReport> const report;
    mir::Fd const shutdown_signal;
    std::shared_ptr<platform::wayland::Cursor> cursor;

    std::mutex sink_mutex;
    std::shared_ptr<input::wayland::KeyboardInput> keyboard_sink;
    std::shared_ptr<input::wayland::PointerInput> pointer_sink;
    std::shared_ptr<input::wayland::TouchInput> touch_sink;
    geometry::Point pointer_pos;
    geometry::Displacement pointer_scroll;
    std::chrono::nanoseconds pointer_time;
    std::vector<events::ContactState> touch_contacts;
    std::chrono::nanoseconds touch_time;

    std::thread runner;
    void run() const;
    void stop();
    auto get_touch_contact(int32_t id) -> decltype(touch_contacts)::iterator;
};

}
}

}

#endif  // MIR_PLATFORMS_WAYLAND_DISPLAY_H_
