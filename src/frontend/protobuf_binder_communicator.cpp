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
#include "binder_service.h"

#include "mir/protobuf/google_protobuf_guard.h"

#include <binder/ProcessState.h>

#include <stdexcept>

namespace mf = mir::frontend;
namespace mfd = mir::frontend::detail;

namespace
{
    android::sp<mfd::BinderService> const session(new mfd::BinderService());
}

mf::ProtobufBinderCommunicator::ProtobufBinderCommunicator(
    const std::string& name,
    std::shared_ptr<ProtobufIpcFactory> const& ipc_factory) :
    ipc_factory(ipc_factory)
{
    session->set_ipc_factory(ipc_factory);

    auto const& sm = android::defaultServiceManager();

    android::String16 const service_name(name.c_str());

    if (sm->checkService(service_name) == 0 &&
        sm->addService(service_name, session) != 0)
    {
        throw std::runtime_error("Failed to add a new Binder service");
    }
}

mf::ProtobufBinderCommunicator::~ProtobufBinderCommunicator()
{
    session->set_ipc_factory(std::shared_ptr<ProtobufIpcFactory>());
}

void mf::ProtobufBinderCommunicator::start()
{
    android::ProcessState::self()->startThreadPool();
}
