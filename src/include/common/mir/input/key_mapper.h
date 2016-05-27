/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *   Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_INPUT_KEY_MAPPER_H_
#define MIR_INPUT_KEY_MAPPER_H_

#include "mir_toolkit/client_types.h"
#include "mir_toolkit/event.h"

#include <xkbcommon/xkbcommon.h>
#include <vector>
#include <memory>

struct MirKeyEvent;

namespace mir
{
namespace input
{
class Keymap;

class KeyMapper
{
public:
    KeyMapper() = default;
    virtual ~KeyMapper() = default;

    /// Update the key state of device \a id, with the given sequence of pressed scan codes.
    virtual void set_key_state(MirInputDeviceId id, std::vector<uint32_t> const& key_state) = 0;

    /** 
     * Per device keymap configuration
     *
     * A keymap can be set for individual input devices by specifying the
     * MirInputDeviceId of that device. A keymap may be supplied as a buffer
     * that can be parsed by xkbcommon, or as tuple of configuration strings
     * used to select the Keymap.
     * \{
     */
    virtual void set_keymap(MirInputDeviceId id, Keymap const& map) = 0;
    virtual void set_keymap(MirInputDeviceId id, char const* buffer, size_t len) = 0;
    virtual void reset_keymap(MirInputDeviceId id) = 0;
    /**
     * \}
     */

    /**
     * Device independent keymap
     *
     * A keymap independent of the input device id can be specified and will
     * override any per device keymaps.
     * \{
     */
    virtual void set_keymap(Keymap const& map) = 0;
    virtual void set_keymap(char const* buffer, size_t len) = 0;
    virtual void reset_keymap() = 0;
    /**
     * \}
     */

    virtual void map_event(MirEvent& event) = 0;

protected:
    KeyMapper(KeyMapper const&) = delete;
    KeyMapper& operator=(KeyMapper const&) = delete;

};

}
}

#endif
