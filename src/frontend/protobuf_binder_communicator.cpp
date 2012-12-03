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

#include "protobuf_binder_communicator.h"


namespace mf = mir::frontend;
namespace mfd = mir::frontend::detail;


mf::ProtobufBinderCommunicator::ProtobufBinderCommunicator(
        const std::string& /*name*/,
        std::shared_ptr<ProtobufIpcFactory> const& /*ipc_factory*/) :
    ipc_factory(ipc_factory)
{
    // TODO
}

mf::ProtobufBinderCommunicator::~ProtobufBinderCommunicator()
{
    // TODO
}

void mf::ProtobufBinderCommunicator::start()
{
    // TODO
}
