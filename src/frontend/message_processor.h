/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
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
 */


#ifndef MIR_FRONTEND_MESSAGE_PROCESSOR_H_
#define MIR_FRONTEND_MESSAGE_PROCESSOR_H_

#include <vector>
#include <memory>
#include <iosfwd>
#include <cstdint>

namespace mir
{
namespace frontend
{
namespace detail
{
class ProtobufMessageProcessor;

struct Sender
{
    virtual void send(const std::ostringstream& buffer2) = 0;
    virtual void send_fds(std::vector<int32_t> const& fd) = 0;
};

struct MessageProcessor
{
    virtual bool process_message(std::istream& msg) = 0;
};

struct NullMessageProcessor : MessageProcessor
{
    bool process_message(std::istream& ) { return false; }
};

}
}
}



#endif /* PROTOBUF_MESSAGE_PROCESSOR_H_ */
