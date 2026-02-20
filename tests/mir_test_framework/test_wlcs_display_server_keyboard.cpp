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

#include <miral/test_wlcs_display_server.h>

#include <wlcs/display_server.h>

#if WLCS_DISPLAY_SERVER_VERSION >= 4

#include "test_wlcs_display_server_internals.h"
#include <wlcs/keyboard.h>

#include <mir/executor.h>
#include <mir/input/device.h>
#include <mir/input/input_device_hub.h>
#include <mir/input/input_device_info.h>
#include <mir/server.h>
#include <mir/test/null_input_device_observer.h>
#include <mir_test_framework/stub_server_platform_factory.h>

namespace mtf = mir_test_framework;
namespace mi = mir::input;

namespace
{

struct FakeKeyboard : public WlcsKeyboard
{
    FakeKeyboard();

    decltype(mtf::add_fake_input_device(mi::InputDeviceInfo())) keyboard;
    miral::TestWlcsDisplayServer* runner;
};

void wlcs_destroy_keyboard(WlcsKeyboard* keyboard)
{
    delete static_cast<FakeKeyboard*>(keyboard);
}

void wlcs_keyboard_key_down(WlcsKeyboard* keyboard, int key_code)
{
    auto device = static_cast<FakeKeyboard*>(keyboard);

    auto event = mir::input::synthesis::a_key_down_event()
                    .of_scancode(key_code);

    emit_mir_event(device->runner, device->keyboard, event);
}

void wlcs_keyboard_key_up(WlcsKeyboard* keyboard, int key_code)
{
    auto device = static_cast<FakeKeyboard*>(keyboard);

    auto event = mir::input::synthesis::a_key_up_event()
                    .of_scancode(key_code);

    emit_mir_event(device->runner, device->keyboard, event);
}
} // namespace

FakeKeyboard::FakeKeyboard()
{
    version = WLCS_KEYBOARD_VERSION;

    key_down = &wlcs_keyboard_key_down;
    key_up = &wlcs_keyboard_key_up;

    destroy = &wlcs_destroy_keyboard;
}

WlcsKeyboard* miral::TestWlcsDisplayServer::create_keyboard()
{
    auto constexpr uid = "keyboard-uid";

    class DeviceObserver : public mir::input::NullInputDeviceObserver
    {
    public:
        explicit DeviceObserver(std::shared_ptr<mir::test::Signal> const& done)
            : done{done}
        {
        }

        void device_added(std::shared_ptr<mir::input::Device> const& device) override
        {
            if (device->unique_id() == uid)
                seen_device = true;
        }

        void changes_complete() override
        {
            if (seen_device)
                done->raise();
        }

    private:
        std::shared_ptr<mir::test::Signal> const done;
        bool seen_device{false};
    };

    auto keyboard_added = std::make_shared<mir::test::Signal>();
    auto observer = std::make_shared<DeviceObserver>(keyboard_added);
    mir_server->the_input_device_hub()->add_observer(observer);

    auto fake_keyboard_dev = mtf::add_fake_input_device(
        mi::InputDeviceInfo{"keyboard", uid, mi::DeviceCapability::keyboard});

    keyboard_added->wait_for(a_long_time);
    executor->spawn([observer=std::move(observer), the_input_device_hub=mir_server->the_input_device_hub()]
                        { the_input_device_hub->remove_observer(observer); });

    auto fake_keyboard = new FakeKeyboard{};
    fake_keyboard->runner = this;
    fake_keyboard->keyboard = std::move(fake_keyboard_dev);

    return static_cast<WlcsKeyboard*>(fake_keyboard);
}

#else

// Just so the compiler doesn't complain about `create_keyboard` not being
// defined.
WlcsKeyboard* miral::TestWlcsDisplayServer::create_keyboard()
{
    std::unreachable();
}

#endif // WLCS_DISPLAY_SERVER_VERSION >= 4
