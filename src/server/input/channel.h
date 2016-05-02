/*
 * Copyright © 2013,2016 Canonical Ltd.
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
 *              Brandon Schaefer <brandon.schaefer@canonical.com>
 */

#ifndef MIR_INPUT_CHANNEL_H_
#define MIR_INPUT_CHANNEL_H_

#include "mir/fd.h"
#include "mir/input/input_channel.h"

namespace mir
{
namespace input
{

class Channel : public InputChannel
{
public:
    Channel();
    virtual ~Channel() override = default;

    int client_fd() const override;
    int server_fd() const override;

private:
    Fd server_fd_;
    Fd client_fd_;
};

}
}

#endif /* MIR_INPUT_CHANNEL_H_ */
