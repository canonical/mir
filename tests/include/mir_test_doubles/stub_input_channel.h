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

#ifndef MIR_TEST_DOUBLES_STUB_INPUT_CHANNEL_H_
#define MIR_TEST_DOUBLES_STUB_INPUT_CHANNEL_H_

#include "mir/input/input_channel.h"
#include "mir/fd.h"

namespace mir
{
namespace test
{
namespace doubles
{

struct StubInputChannel : public input::InputChannel
{
    StubInputChannel(int fd)
      : input_fd(fd)
    {
    }

    StubInputChannel()
     : StubInputChannel(Fd::invalid)
    {
    }

    int client_fd() const override
    {
        return input_fd;
    }
    int server_fd() const override
    {
        return input_fd;
    }
    Fd input_fd;
};

}
}
} // namespace mir

#endif // MIR_TEST_DOUBLES_STUB_INPUT_CHANNEL_H_

