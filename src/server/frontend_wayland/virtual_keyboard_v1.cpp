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
#include "wayland.h"

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
#include <mir/fd.h>

#include <cstring>
#include <sys/mman.h>
#include <boost/throw_exception.hpp>

namespace mf = mir::frontend;
namespace mw = mir::wayland_rs;
namespace mi = mir::input;

namespace
{
auto mir_keyboard_action(uint32_t wayland_state) -> MirKeyboardAction
{
    switch (wayland_state)
    {
    case mw::WlKeyboardImpl::KeyState::pressed:
        return mir_keyboard_action_down;
    case mw::WlKeyboardImpl::KeyState::released:
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
    if (format != mw::WlKeyboardImpl::KeymapFormat::xkb_v1)
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

class VirtualKeyboardV1
    : public mw::ZwpVirtualKeyboardV1Impl
{
public:
    VirtualKeyboardV1(std::shared_ptr<VirtualKeyboardV1Ctx> const& ctx);
    ~VirtualKeyboardV1();

    void keymap(uint32_t format, int32_t fd, uint32_t size) override;
    void key(uint32_t time, uint32_t key, uint32_t state) override;
    void modifiers(uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group) override;
private:

    std::shared_ptr<VirtualKeyboardV1Ctx> const ctx;
    std::shared_ptr<input::VirtualInputDevice> const keyboard_device;
    std::weak_ptr<input::Device> const device_handle;
    std::optional<MirXkbModifiers> xkb_modifiers;
};
}
}

mf::VirtualKeyboardManagerV1::VirtualKeyboardManagerV1(
    std::shared_ptr<VirtualKeyboardV1Ctx> const& ctx)
    : ctx{ctx}
{
}

auto mf::VirtualKeyboardManagerV1::create_virtual_keyboard(wayland_rs::Weak<wayland_rs::WlSeatImpl> const&) -> std::shared_ptr<wayland_rs::ZwpVirtualKeyboardV1Impl>
{
    return std::make_shared<VirtualKeyboardV1>(ctx);
}

mf::VirtualKeyboardV1::VirtualKeyboardV1(
    std::shared_ptr<VirtualKeyboardV1Ctx> const& ctx)
    : ctx{ctx},
      keyboard_device{std::make_shared<mi::VirtualInputDevice>("virtual-keyboard", mi::DeviceCapability::keyboard)},
      device_handle{ctx->device_registry->add_device(keyboard_device)}
{
}

mf::VirtualKeyboardV1::~VirtualKeyboardV1()
{
    ctx->device_registry->remove_device(keyboard_device);
}

void mf::VirtualKeyboardV1::keymap(uint32_t format, int32_t fd, uint32_t size)
{
    if (auto const device = device_handle.lock())
    {
        auto keymap = load_keymap(format, mir::Fd(fd), size);
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
