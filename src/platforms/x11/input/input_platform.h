/*
 * Copyright © 2015 Canonical Ltd.
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
 *
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */
#ifndef MIR_INPUT_X_INPUT_PLATFORM_H_
#define MIR_INPUT_X_INPUT_PLATFORM_H_

#include "mir/input/platform.h"
#include <memory>
#include <functional>
#include <vector>
#include <xcb/xcb.h>

struct xkb_context;
struct xkb_keymap;
struct xkb_state;

namespace mir
{

namespace dispatch
{
class ReadableFd;
}

namespace X
{
class X11Resources;
}

namespace input
{

namespace X
{
class XInputDevice;

class XInputPlatform : public input::Platform
{
public:
    explicit XInputPlatform(
        std::shared_ptr<input::InputDeviceRegistry> const& input_device_registry,
        std::shared_ptr<mir::X::X11Resources> const& x11_resources);
    ~XInputPlatform();

    std::shared_ptr<dispatch::Dispatchable> dispatchable() override;
    void start() override;
    void stop() override;
    void pause_for_config() override;
    void continue_after_config() override;

private:
    void process_input_events();
    void process_input_event(xcb_generic_event_t* event);
    /// Defer work until all pending events are processed. Should only be called while processing events.
    void defer(std::function<void()>&& work);
    /// Defer work until after processing the current event. Called with nullopt if the current event is the last
    /// event that is currently pending (doesn't wait for new events). Should only be called while processing events.
    /// Given function returns true if it consumes the event (otherwise the process_input_event() is then called).
    void with_next_pending_event(std::function<bool(std::optional<xcb_generic_event_t*> event)>&& work);
    std::shared_ptr<mir::X::X11Resources> const x11_resources;
    std::shared_ptr<dispatch::ReadableFd> const xcon_dispatchable;
    std::shared_ptr<input::InputDeviceRegistry> const registry;
    std::shared_ptr<XInputDevice> const core_keyboard;
    std::shared_ptr<XInputDevice> const core_pointer;
    xkb_context* const xkb_ctx;
    xkb_keymap* const keymap;
    xkb_state* const key_state;
    bool kbd_grabbed;
    bool ptr_grabbed;
    std::vector<std::function<void()>> deferred;
    std::vector<std::function<bool(std::optional<xcb_generic_event_t*> event)>> next_pending_event_callbacks;
};

}
}
}

#endif // MIR_INPUT_X_INPUT_PLATFORM_H_
