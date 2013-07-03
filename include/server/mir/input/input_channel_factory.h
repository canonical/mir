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

#ifndef MIR_INPUT_INPUT_CHANNEL_FACTORY_H_
#define MIR_INPUT_INPUT_CHANNEL_FACTORY_H_

#include <memory>

namespace mir
{
namespace input
{
class InputChannel;

class InputChannelFactory
{
public:
    virtual ~InputChannelFactory() {}

    virtual std::shared_ptr<InputChannel> make_input_channel() = 0;

protected:
    InputChannelFactory() = default;
    InputChannelFactory(InputChannelFactory const&) = delete;
    InputChannelFactory& operator=(InputChannelFactory const&) = delete;
};

}
} // namespace mir

#endif // MIR_INPUT_INPUT_CHANNEL_FACTORY_H_
