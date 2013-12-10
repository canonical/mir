/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *   Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_CLIENT_MESA_CLIENT_BUFFER_FACTORY_H_
#define MIR_CLIENT_MESA_CLIENT_BUFFER_FACTORY_H_

#include "../client_buffer_factory.h"

namespace mir
{
namespace client
{
namespace mesa
{

class BufferFileOps;

class ClientBufferFactory : public client::ClientBufferFactory
{
public:
    explicit ClientBufferFactory(std::shared_ptr<BufferFileOps> const& buffer_file_ops);

    std::shared_ptr<client::ClientBuffer> create_buffer(
        std::shared_ptr<MirBufferPackage> const& package,
        geometry::Size size, MirPixelFormat pf);
private:
    std::shared_ptr<BufferFileOps> const buffer_file_ops;
};

}
}
}
#endif /* MIR_CLIENT_MESA_CLIENT_BUFFER_FACTORY_H_ */
