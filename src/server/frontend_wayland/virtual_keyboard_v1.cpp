/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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

#include "virtual_keyboard_v1.h"
#include "wayland_wrapper.h"

#include <mir/input/event_builder.h>
#include <mir/input/input_sink.h>
#include <mir/input/input_device_registry.h>
#include <mir/input/device.h>
#include <mir/input/virtual_input_device.h>
#include <mir/input/mir_keyboard_config.h>
#include <mir/input/buffer_keymap.h>
#include <mir/events/xkb_modifiers.h>
#include <mir/events/event.h>
#include <mir/events/input_event.h>
#include <mir/events/keyboard_event.h>
#include <mir/log.h>

#include <cstring>
#include <sys/mman.h>
#include <boost/throw_exception.hpp>

namespace mf = mir::frontend;
namespace mw = mir::wayland;
namespace mi = mir::input;

namespace
{
auto mir_keyboard_action(uint32_t wayland_state) -> MirKeyboardAction
{
    switch (wayland_state)
    {
    case mw::Keyboard::KeyState::pressed:
        return mir_keyboard_action_down;
    case mw::Keyboard::KeyState::released:
        return mir_keyboard_action_up;
    default:
        // Protocol does not provide an appropriate error code, so throw a generic runtime_error. This will be expressed
        // to the client as an implementation error
        BOOST_THROW_EXCEPTION(std::runtime_error(
            "Invalid virtual keyboard key state " + std::to_string(wayland_state)));
    }
}

auto load_keymap(uint32_t format, mir::Fd fd, size_t size) -> std::shared_ptr<mi::Keymap>
{
    if (format != mw::Keyboard::KeymapFormat::xkb_v1)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("invalid keymap format " + std::to_string(format)));
    }

    void* const data = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED)
    {
        BOOST_THROW_EXCEPTION(std::system_error(errno, std::system_category(), "failed to mmap keymap fd"));
    }
    std::vector<char> buffer(size);
    memcpy(buffer.data(), data, size);
    munmap(data, size);

    // Keymaps are null-terminated, BufferKeymap does not expect a null-terminated buffer
    while (buffer.back() == '\0')
    {
        buffer.pop_back();
    }

    return std::make_shared<mi::BufferKeymap>("virtual-keyboard-keymap", std::move(buffer), XKB_KEYMAP_FORMAT_TEXT_V1);
}
}

namespace mir
{
namespace frontend
{
struct VirtualKeyboardV1Ctx
{
    std::shared_ptr<mi::InputDeviceRegistry> const device_registry;
};

class VirtualKeyboardManagerV1Global
    : public wayland::VirtualKeyboardManagerV1::Global
{
public:
    VirtualKeyboardManagerV1Global(wl_display* display, std::shared_ptr<VirtualKeyboardV1Ctx> const& ctx);

private:
    void bind(wl_resource* new_resource) override;

    std::shared_ptr<VirtualKeyboardV1Ctx> const ctx;
};

class VirtualKeyboardManagerV1
    : public wayland::VirtualKeyboardManagerV1
{
public:
    VirtualKeyboardManagerV1(wl_resource* resource, std::shared_ptr<VirtualKeyboardV1Ctx> const& ctx);

private:
    void create_virtual_keyboard(struct wl_resource* seat, struct wl_resource* id) override;

    std::shared_ptr<VirtualKeyboardV1Ctx> const ctx;
};

class VirtualKeyboardV1
    : public wayland::VirtualKeyboardV1
{
public:
    VirtualKeyboardV1(wl_resource* resource, std::shared_ptr<VirtualKeyboardV1Ctx> const& ctx);
    ~VirtualKeyboardV1();

private:
    void keymap(uint32_t format, mir::Fd fd, uint32_t size) override;
    void key(uint32_t time, uint32_t key, uint32_t state) override;
    void modifiers(uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group) override;

    std::shared_ptr<VirtualKeyboardV1Ctx> const ctx;
    std::shared_ptr<input::VirtualInputDevice> const keyboard_device;
    std::weak_ptr<input::Device> const device_handle;
    std::optional<MirXkbModifiers> xkb_modifiers;
};
}
}

auto mf::create_virtual_keyboard_manager_v1(
    wl_display* display,
    std::shared_ptr<mi::InputDeviceRegistry> const& device_registry)
-> std::shared_ptr<mw::VirtualKeyboardManagerV1::Global>
{
    auto ctx = std::shared_ptr<VirtualKeyboardV1Ctx>{new VirtualKeyboardV1Ctx{device_registry}};
    return std::make_shared<VirtualKeyboardManagerV1Global>(display, std::move(ctx));
}

mf::VirtualKeyboardManagerV1Global::VirtualKeyboardManagerV1Global(
    wl_display* display,
    std::shared_ptr<VirtualKeyboardV1Ctx> const& ctx)
    : Global{display, Version<1>()},
      ctx{ctx}
{
}

void mf::VirtualKeyboardManagerV1Global::bind(wl_resource* new_resource)
{
    new VirtualKeyboardManagerV1{new_resource, ctx};
}

mf::VirtualKeyboardManagerV1::VirtualKeyboardManagerV1(
    wl_resource* resource,
    std::shared_ptr<VirtualKeyboardV1Ctx> const& ctx)
    : wayland::VirtualKeyboardManagerV1{resource, Version<1>()},
      ctx{ctx}
{
}

void mf::VirtualKeyboardManagerV1::create_virtual_keyboard(struct wl_resource* seat, struct wl_resource* id)
{
    (void)seat;
    new VirtualKeyboardV1{id, ctx};
}

mf::VirtualKeyboardV1::VirtualKeyboardV1(
    wl_resource* resource,
    std::shared_ptr<VirtualKeyboardV1Ctx> const& ctx)
    : wayland::VirtualKeyboardV1{resource, Version<1>()},
      ctx{ctx},
      keyboard_device{std::make_shared<mi::VirtualInputDevice>("virtual-keyboard", mi::DeviceCapability::keyboard)},
      device_handle{ctx->device_registry->add_device(keyboard_device)}
{
}

mf::VirtualKeyboardV1::~VirtualKeyboardV1()
{
    ctx->device_registry->remove_device(keyboard_device);
}

void mf::VirtualKeyboardV1::keymap(uint32_t format, mir::Fd fd, uint32_t size)
{
    if (auto const device = device_handle.lock())
    {
        auto keymap = load_keymap(format, std::move(fd), size);
        MirKeyboardConfig const config{std::move(keymap)};
        device->apply_keyboard_configuration(config);
    }
}

void mf::VirtualKeyboardV1::key(uint32_t time, uint32_t key, uint32_t state)
{
    std::chrono::nanoseconds nano = std::chrono::milliseconds{time};
    keyboard_device->if_started_then([&](input::InputSink* sink, input::EventBuilder* builder)
        {
            auto key_event = builder->key_event(nano, mir_keyboard_action(state), 0, key);
            key_event->to_input()->to_keyboard()->set_xkb_modifiers(xkb_modifiers);
            sink->handle_input(std::move(key_event));
        });
}

void mf::VirtualKeyboardV1::modifiers(
    uint32_t mods_depressed,
    uint32_t mods_latched,
    uint32_t mods_locked,
    uint32_t group)
{
    xkb_modifiers = MirXkbModifiers{
        mods_depressed,
        mods_latched,
        mods_locked,
        group};
    std::chrono::nanoseconds nano = std::chrono::steady_clock::now().time_since_epoch();
    keyboard_device->if_started_then([&](input::InputSink* sink, input::EventBuilder* builder)
        {
            auto key_event = builder->key_event(nano, mir_keyboard_action_modifiers, 0, 0);
            key_event->to_input()->to_keyboard()->set_xkb_modifiers(xkb_modifiers);
            sink->handle_input(std::move(key_event));
        });
}
