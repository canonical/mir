/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_TEST_DOUBLES_KEY_MAPPER_H_
#define MIR_TEST_DOUBLES_KEY_MAPPER_H_

#include "mir/input/key_mapper.h"
#include "mir/input/keymap.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{
struct MockKeyMapper : input::KeyMapper
{
    MOCK_METHOD(void, set_key_state, (MirInputDeviceId id, std::vector<uint32_t> const& key_state), (override));
    MOCK_METHOD(void, set_keymap_for_device, (MirInputDeviceId id, std::shared_ptr<input::Keymap> map), (override));
    MOCK_METHOD(void, set_keymap_for_all_devices, (std::shared_ptr<input::Keymap> map), (override));
    MOCK_METHOD(void, clear_keymap_for_device, (MirInputDeviceId id), (override));
    MOCK_METHOD(void, clear_all_keymaps, (), (override));
    MOCK_METHOD(void, map_event, (MirEvent& event), (override));
    MOCK_METHOD(MirInputEventModifiers, modifiers, (), (const, override));
    MOCK_METHOD(MirInputEventModifiers, device_modifiers, (MirInputDeviceId), (const, override));
    MOCK_METHOD(MirXkbModifiers, xkb_modifiers, (), (const, override));
};

}
}
}

#endif
