/*
 * Copyright © 2013-2014 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 *              Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "nested_input_configuration.h"
#include "null_input_manager.h"

#include "android/input_channel_factory.h"

#include <memory>

namespace mi = mir::input;
namespace mia = mir::input::android;

mi::NestedInputConfiguration::NestedInputConfiguration()
{
}

std::shared_ptr<mi::InputChannelFactory> mi::NestedInputConfiguration::the_input_channel_factory()
{
    return std::make_shared<mia::InputChannelFactory>();
}

std::shared_ptr<mi::InputManager> mi::NestedInputConfiguration::the_input_manager()
{
    return input_manager(
        [this]()
        {
            return std::make_shared<NullInputManager>();
        });
}
