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
#ifndef MIR_CLIENT_CLIENT_PLATFORM_FACTORY_H_
#define MIR_CLIENT_CLIENT_PLATFORM_FACTORY_H_

namespace mir
{
namespace client
{

class ClientPlatform;
class ClientContext;

class ClientPlatformFactory
{
public:
    virtual ~ClientPlatformFactory() {}

    virtual std::shared_ptr<ClientPlatform> create_client_platform(ClientContext* context) = 0;

protected:
    ClientPlatformFactory() = default;
    ClientPlatformFactory(const ClientPlatformFactory& p) = delete;
    ClientPlatformFactory& operator=(const ClientPlatformFactory& p) = delete;
};

}
}

#endif /* MIR_CLIENT_CLIENT_PLATFORM_FACTORY_H_ */
