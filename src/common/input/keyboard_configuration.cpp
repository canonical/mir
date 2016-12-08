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

#include "mir/input/keyboard_configuration.h"

#include <ostream>

namespace mi = mir::input;

struct mi::KeyboardConfiguration::Implementation
{
    Implementation() = default;
    Implementation(Keymap && keymap)
        : device_keymap(keymap) {}
    Keymap device_keymap;
};

mi::KeyboardConfiguration::KeyboardConfiguration()
    : impl{std::make_unique<Implementation>()}
{}

mi::KeyboardConfiguration::~KeyboardConfiguration() = default;

mi::KeyboardConfiguration::KeyboardConfiguration(Keymap && keymap)
    : impl{std::make_unique<Implementation>(std::move(keymap))}
{
}

mi::KeyboardConfiguration::KeyboardConfiguration(KeyboardConfiguration && other)
    : impl{std::move(other.impl)}
{
}

mi::KeyboardConfiguration::KeyboardConfiguration(KeyboardConfiguration const& other)
    : impl{std::make_unique<Implementation>(*other.impl)}
{
}

mi::KeyboardConfiguration& mi::KeyboardConfiguration::operator=(KeyboardConfiguration const& other)
{
    *impl = *other.impl;
    return *this;
}

mi::Keymap const& mi::KeyboardConfiguration::device_keymap() const
{
    return impl->device_keymap;
}

void mi::KeyboardConfiguration::device_keymap(Keymap const& keymap)
{
    impl->device_keymap = keymap;
}

bool mi::KeyboardConfiguration::operator==(KeyboardConfiguration const& rhs) const
{
    return device_keymap() == rhs.device_keymap();
}

bool mi::KeyboardConfiguration::operator!=(KeyboardConfiguration const& rhs) const
{
    return !(*this == rhs);
}

std::ostream& operator<<(std::ostream& out, mi::KeyboardConfiguration const& keyboard)
{
    return out << " keymap:" << keyboard.device_keymap();
}
