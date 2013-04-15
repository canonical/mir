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

#ifndef MIR_INPUT_INPUT_CHANNEL_H_
#define MIR_INPUT_INPUT_CHANNEL_H_

namespace mir
{
namespace input
{

/// Encapsulates a paired set of fd's suitable for input communication.
class InputChannel
{
public:
    virtual ~InputChannel() {}

    virtual int client_fd() const = 0;
    virtual int server_fd() const = 0;

protected:
    InputChannel() = default;
    InputChannel(InputChannel const&) = delete;
    InputChannel& operator=(InputChannel const&) = delete;
};

}
} // namespace mir

#endif // MIR_INPUT_INPUT_CHANNEL_H_
