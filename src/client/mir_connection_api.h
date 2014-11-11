/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_CLIENT_MIR_CONNECTION_API_H_
#define MIR_CLIENT_MIR_CONNECTION_API_H_

#include "mir_toolkit/mir_wait.h"

#include <string>
#include <memory>
#include <functional>

namespace mir
{
namespace client
{
class ConnectionConfiguration;

using ConfigurationFactory = std::function<std::unique_ptr<ConnectionConfiguration>(std::string const&)>;

class MirConnectionAPI
{
public:
    virtual ~MirConnectionAPI() = default;

    virtual MirWaitHandle* connect(
        ConfigurationFactory configuration,
        char const* socket_file,
        char const* name,
        mir_connected_callback callback,
        void* context) = 0;

    virtual void release(MirConnection* connection) = 0;

    virtual ConfigurationFactory configuration_factory() = 0;

protected:
    MirConnectionAPI() = default;
    MirConnectionAPI(MirConnectionAPI const&) = default;
    MirConnectionAPI& operator=(MirConnectionAPI const&) = default;
};

}
}

extern "C" mir::client::MirConnectionAPI* mir_connection_api_impl;

#endif
