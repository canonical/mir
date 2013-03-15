/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_FRONTEND_PROTOBUF_BINDER_COMMUNICATOR_H_
#define MIR_FRONTEND_PROTOBUF_BINDER_COMMUNICATOR_H_

#include "mir/frontend/communicator.h"

#include <memory>
#include <string>

namespace mir
{
namespace frontend
{
class ProtobufIpcFactory;


class ProtobufBinderCommunicator : public Communicator
{
public:
    explicit ProtobufBinderCommunicator(
        const std::string& name,
        std::shared_ptr<ProtobufIpcFactory> const& ipc_factory);
    ~ProtobufBinderCommunicator();
    void start();

private:

    std::shared_ptr<ProtobufIpcFactory> const ipc_factory;
};

}
}

#endif // MIR_FRONTEND_PROTOBUF_BINDER_COMMUNICATOR_H_
