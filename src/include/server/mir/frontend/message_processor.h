/*
 * Copyright © 2012, 2014 Canonical Ltd.
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
 */

#ifndef MIR_FRONTEND_MESSAGE_PROCESSOR_H_
#define MIR_FRONTEND_MESSAGE_PROCESSOR_H_

#include <google/protobuf/stubs/common.h>

#include <mir/fd.h>
#include <vector>

namespace mir
{
namespace protobuf
{
namespace wire
{
class Invocation;
}
}
namespace frontend
{
namespace detail
{
class Invocation
{
public:
    Invocation(mir::protobuf::wire::Invocation const& invocation) :
        invocation(invocation) {}

    const ::std::string& method_name() const;
    const ::std::string& parameters() const;
    google::protobuf::uint32 id() const;
private:
    mir::protobuf::wire::Invocation const& invocation;
};

class MessageProcessor
{
public:
    virtual bool dispatch(Invocation const& invocation, std::vector<mir::Fd> const& side_channel_fds) = 0;
    virtual void client_pid(int pid) = 0;

protected:
    MessageProcessor() = default;
    virtual ~MessageProcessor() = default;
    MessageProcessor(MessageProcessor const&) = delete;
    MessageProcessor& operator=(MessageProcessor const&) = delete;
};
}
}
}
#endif /* PROTOBUF_MESSAGE_PROCESSOR_H_ */
