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


#ifndef MIR_FRONTEND_BINDER_SERVICE_H_
#define MIR_FRONTEND_BINDER_SERVICE_H_

#include "mir/thread/all.h"

#include <binder/Binder.h>

#include <map>
#include <memory>

namespace mir
{
namespace protobuf { class DisplayServer; }
namespace frontend
{
class ProtobufIpcFactory;
namespace detail
{
class BinderCallContext;

class BinderService : public android::BBinder
{
public:
    BinderService();
    ~BinderService();

    void set_ipc_factory(std::shared_ptr<ProtobufIpcFactory> const& ipc_factory);

private:
    android::status_t onTransact(uint32_t code,
                                 const android::Parcel& request,
                                 android::Parcel* response,
                                 uint32_t flags);

    std::shared_ptr<protobuf::DisplayServer> get_session_for(pid_t client_pid);
    void close_session_for(pid_t client_pid);

    std::mutex guard;
    std::shared_ptr<ProtobufIpcFactory> ipc_factory;
    std::map<pid_t, std::shared_ptr<protobuf::DisplayServer>> sessions;
};
}
}
}


#endif /* MIR_FRONTEND_BINDER_SERVICE_H_ */
