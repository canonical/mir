/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "rudimentary_input_reader_policy.h"

namespace mia = mir::input::android;

void mia::RudimentaryInputReaderPolicy::getReaderConfiguration(droidinput::InputReaderConfiguration* out_config)
{
    (void)out_config;
}

droidinput::sp<droidinput::PointerControllerInterface> mia::RudimentaryInputReaderPolicy::obtainPointerController(int32_t device_id)
{
    (void)device_id;
    return pointer_controller;
}

void mia::RudimentaryInputReaderPolicy::notifyInputDevicesChanged(
    const droidinput::Vector<droidinput::InputDeviceInfo>& input_devices)
{
    (void)input_devices;
}

droidinput::sp<droidinput::KeyCharacterMap> mia::RudimentaryInputReaderPolicy::getKeyboardLayoutOverlay(
    const droidinput::String8& input_device_descriptor)
{
    (void)input_device_descriptor;
    return droidinput::KeyCharacterMap::empty();
}

droidinput::String8 mia::RudimentaryInputReaderPolicy::getDeviceAlias(const droidinput::InputDeviceIdentifier& identifier)
{
    (void)identifier;
    return droidinput::String8();
}
