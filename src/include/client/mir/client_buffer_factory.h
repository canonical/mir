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

#ifndef MIR_CLIENT_CLIENT_BUFFER_FACTORY_H_
#define MIR_CLIENT_CLIENT_BUFFER_FACTORY_H_

#include <memory>

#include "mir_toolkit/mir_client_library.h"
#include "mir_toolkit/common.h"
#include "mir/geometry/size.h"

namespace mir
{
namespace client
{
class ClientBuffer;

/**
 * A factory for creating client-side representations of graphics buffers.
 */
class ClientBufferFactory
{
public:
    /**
     * Creates the client-side representation of a buffer.
     *
     * \param [in] package the buffer package sent by the server for this buffer
     * \param [in] size the buffer's size
     * \param [in] pf the buffer's pixel format
     */
    virtual std::shared_ptr<ClientBuffer> create_buffer(std::shared_ptr<MirBufferPackage> const& package,
                                                        geometry::Size size, MirPixelFormat pf) = 0;

protected:
    ClientBufferFactory() = default;
    ClientBufferFactory(ClientBufferFactory const &) = delete;
    ClientBufferFactory &operator=(ClientBufferFactory const &) = delete;
    virtual ~ClientBufferFactory() {}
};

}
}
#endif /* MIR_CLIENT_CLIENT_BUFFER_FACTORY_H_ */
