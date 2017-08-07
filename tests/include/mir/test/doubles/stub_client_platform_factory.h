/*
 * Copyright © 2017 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_STUB_CLIENT_PLATFORM_FACTORY_H_
#define MIR_TEST_DOUBLES_STUB_CLIENT_PLATFORM_FACTORY_H_

#include "mir/client/client_platform_factory.h"

namespace mir
{
namespace test
{
namespace doubles
{

struct StubClientPlatformFactory : public client::ClientPlatformFactory
{
    StubClientPlatformFactory(std::shared_ptr<client::ClientPlatform> const& platform)
        : platform{platform}
    {
    }

    std::shared_ptr<client::ClientPlatform> create_client_platform(client::ClientContext*)
    {
        return platform;
    }

    std::shared_ptr<client::ClientPlatform> platform;
};

}
}
}

#endif /* MIR_TEST_DOUBLES_STUB_CLIENT_PLATFORM_FACTORY_H_ */
