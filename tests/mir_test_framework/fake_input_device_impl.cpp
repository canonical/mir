/*
 * Copyright © 2015 Canonical Ltd.
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
 *
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#define MIR_INCLUDE_DEPRECATED_EVENT_HEADER

#include "fake_input_device_impl.h"
#include "stub_input_platform.h"

#include "mir/input/input_event_handler_register.h"
#include "mir/input/input_device.h"
#include "mir/input/input_sink.h"

#include "mir_toolkit/events/event.h"


namespace mi = mir::input;
namespace mtf = mir_test_framework;

mtf::FakeInputDeviceImpl::FakeInputDeviceImpl(mi::InputDeviceInfo const& info)
    : device{std::make_shared<InputDevice>(info)}
{
    mtf::StubInputPlatform::add(device);
}

mtf::FakeInputDeviceImpl::~FakeInputDeviceImpl()
{
    mtf::StubInputPlatform::remove(device);
}

void mtf::FakeInputDeviceImpl::emit_event(MirEvent const& input_event)
{
    device->emit_event(input_event);
}

void mtf::FakeInputDeviceImpl::InputDevice::emit_event(MirEvent const& input_event)
{
    std::unique_lock<std::mutex> lock(mutex);
    if (!event_handler)
    {
        //BOOST_THROW_EXCEPTION();
    }

    event_handler->register_handler(
        [this, input_event]()
        {
            std::unique_lock<std::mutex> lock(mutex);
            if (sink)
            {
                MirEvent copy = input_event;
                sink->handle_input(copy);
            }
        });
}

mtf::FakeInputDeviceImpl::InputDevice::InputDevice(mi::InputDeviceInfo const& info)
    : info(info)
{
}

void mtf::FakeInputDeviceImpl::InputDevice::start(mi::InputEventHandlerRegister& registry, mi::InputSink& destination)
{
    std::unique_lock<std::mutex> lock(mutex);
    event_handler = &registry;
    sink = &destination;
}

void mtf::FakeInputDeviceImpl::InputDevice::stop(mi::InputEventHandlerRegister&)
{
    std::unique_lock<std::mutex> lock(mutex);
    event_handler = nullptr;
    sink = nullptr;
}
