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

#ifndef MIR_INPUT_KEYBOARD_CONFIGURATION_H_
#define MIR_INPUT_KEYBOARD_CONFIGURATION_H_

#include "mir_toolkit/common.h"
#include "mir_toolkit/mir_input_device_types.h"
#include "mir/input/keymap.h"

#include <iosfwd>
#include <memory>

/*
 * Keyboard device configuration.
 */
struct MirKeyboardConfiguration
{
    MirKeyboardConfiguration();
    ~MirKeyboardConfiguration();
    MirKeyboardConfiguration(mir::input::Keymap&& keymap);
    MirKeyboardConfiguration(MirKeyboardConfiguration&& other);
    MirKeyboardConfiguration(MirKeyboardConfiguration const& other);
    MirKeyboardConfiguration& operator=(MirKeyboardConfiguration const& other);

    mir::input::Keymap const& device_keymap() const;
    void device_keymap(mir::input::Keymap const& );

    bool operator==(MirKeyboardConfiguration const& rhs) const;
    bool operator!=(MirKeyboardConfiguration const& rhs) const;
private:
    struct Implementation;
    std::unique_ptr<Implementation> impl;
};

std::ostream& operator<<(std::ostream& out, MirKeyboardConfiguration const& keyboard);

#endif
