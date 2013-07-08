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
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_CLIENT_NATIVE_CLIENT_PLATFORM_FACTORY_
#define MIR_CLIENT_NATIVE_CLIENT_PLATFORM_FACTORY_

#include "client_platform_factory.h"

namespace mir
{
namespace client
{

/**
 * Factory for creating the native client platform.
 * \ingroup platform_enablement
 */
class NativeClientPlatformFactory : public ClientPlatformFactory
{
public:
    /**
     * Creates a client platform.
     *
     * This method needs to be implemented by each platform.
     *
     * \param [in] context information about the client
     */
    std::shared_ptr<ClientPlatform> create_client_platform(ClientContext* context);
};

}
}

#endif /* MIR_CLIENT_NATIVE_CLIENT_PLATFORM_FACTORY_ */
