/*
 * Copyright © 2014,2016 Canonical Ltd.
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

#ifndef MIR_INPUT_CHANNEL_FACTORY_H_
#define MIR_INPUT_CHANNEL_FACTORY_H_

#include "mir/input/input_channel_factory.h"

namespace mir
{
namespace input
{

class ChannelFactory : public InputChannelFactory
{
public:
    virtual std::shared_ptr<InputChannel> make_input_channel() override;

    ChannelFactory() = default;
    ChannelFactory(ChannelFactory const&) = delete;
    ChannelFactory& operator=(ChannelFactory const&) = delete;
};

}
}

#endif /* MIR_INPUT_CHANNEL_FACTORY_H_ */
