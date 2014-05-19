/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "null_input_configuration.h"
#include "null_input_manager.h"

#include "mir/input/input_channel_factory.h"
#include "mir/input/input_channel.h"

namespace mi = mir::input;

namespace
{

class NullInputChannel : public mi::InputChannel
{
    int client_fd() const override
    {
        return 0;
    }
    int server_fd() const override
    {
        return 0;
    }
};

class NullInputChannelFactory : public mi::InputChannelFactory
{
    std::shared_ptr<mi::InputChannel> make_input_channel() override
    {
        return std::make_shared<NullInputChannel>();
    }
};

}

std::shared_ptr<mi::InputChannelFactory> mi::NullInputConfiguration::the_input_channel_factory()
{
    return std::make_shared<NullInputChannelFactory>();
}

std::shared_ptr<mi::InputManager> mi::NullInputConfiguration::the_input_manager()
{
    return std::make_shared<NullInputManager>();
}

