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

#include "mir/input/mir_keyboard_configuration.h"

#include <ostream>

namespace mi = mir::input;

struct MirKeyboardConfiguration::Implementation
{
    Implementation() = default;
    Implementation(mi::Keymap && keymap)
        : device_keymap(keymap) {}
    mi::Keymap device_keymap;
};

MirKeyboardConfiguration::MirKeyboardConfiguration()
    : impl{std::make_unique<Implementation>()}
{}

MirKeyboardConfiguration::~MirKeyboardConfiguration() = default;

MirKeyboardConfiguration::MirKeyboardConfiguration(mi::Keymap && keymap)
    : impl{std::make_unique<Implementation>(std::move(keymap))}
{
}

MirKeyboardConfiguration::MirKeyboardConfiguration(MirKeyboardConfiguration && other)
    : impl{std::move(other.impl)}
{
}

MirKeyboardConfiguration::MirKeyboardConfiguration(MirKeyboardConfiguration const& other)
    : impl{std::make_unique<Implementation>(*other.impl)}
{
}

MirKeyboardConfiguration& MirKeyboardConfiguration::operator=(MirKeyboardConfiguration const& other)
{
    *impl = *other.impl;
    return *this;
}

mi::Keymap const& MirKeyboardConfiguration::device_keymap() const
{
    return impl->device_keymap;
}

void MirKeyboardConfiguration::device_keymap(mi::Keymap const& keymap)
{
    impl->device_keymap = keymap;
}

bool MirKeyboardConfiguration::operator==(MirKeyboardConfiguration const& rhs) const
{
    return device_keymap() == rhs.device_keymap();
}

bool MirKeyboardConfiguration::operator!=(MirKeyboardConfiguration const& rhs) const
{
    return !(*this == rhs);
}

std::ostream& operator<<(std::ostream& out, MirKeyboardConfiguration const& keyboard)
{
    return out << " keymap:" << keyboard.device_keymap();
}
