/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_STUB_CLIENT_BUFFER_FACTORY_H_
#define MIR_TEST_DOUBLES_STUB_CLIENT_BUFFER_FACTORY_H_

#include "src/include/client/mir/client_buffer_factory.h"
#include "stub_client_buffer.h"

namespace mir
{
namespace test
{
namespace doubles
{

struct StubClientBufferFactory : public client::ClientBufferFactory
{
    std::shared_ptr<client::ClientBuffer> create_buffer(
        std::shared_ptr<MirBufferPackage> const& package,
        geometry::Size size, MirPixelFormat pf)
    {
        return std::make_shared<StubClientBuffer>(package, size, pf);
    }
};

}
}
}

#endif
