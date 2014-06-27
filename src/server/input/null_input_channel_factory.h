/*
 * Copyright © 2014 Canonical Ltd.
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

#ifndef MIR_INPUT_NULL_INPUT_CHANNEL_FACTORY_H_
#define MIR_INPUT_NULL_INPUT_CHANNEL_FACTORY_H_

#include "mir/input/input_channel_factory.h"

namespace mir
{
namespace input
{
class InputChannel;

class NullInputChannelFactory : public mir::input::InputChannelFactory
{
    std::shared_ptr<mir::input::InputChannel> make_input_channel() override;
};

}
}

#endif
