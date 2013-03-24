/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *   Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_CLIENT_PRIVATE_MIR_CLIENT_BUFFER_DEPOSITORY_H_
#define MIR_CLIENT_PRIVATE_MIR_CLIENT_BUFFER_DEPOSITORY_H_

#include <map>
#include <memory>

#include "mir/geometry/pixel_format.h"
#include "mir/geometry/size.h"

namespace mir_toolkit
{
struct MirBufferPackage;
}

namespace mir
{

namespace client
{
class ClientBuffer;
class ClientBufferFactory;

/// Responsible for taking the buffer data sent from the server and wrapping it in a ClientBuffer

/// The ClientBufferDepository is responsible for taking the IPC buffer data and converting it into
/// the more digestible form of a ClientBuffer. It maintains the client-side state necessary to
/// construct a ClientBuffer.
class ClientBufferDepository
{
public:
    ClientBufferDepository(std::shared_ptr<ClientBufferFactory> const &factory, int max_buffers);

    /// Construct a ClientBuffer from the IPC data, and use it as the current buffer.

    /// This also marks the previous current buffer (if any) as being submitted to the server.
    /// \post current_buffer() will return a ClientBuffer constructed from this IPC data.
    void deposit_package(std::shared_ptr<mir_toolkit::MirBufferPackage> const &, int id,
                         geometry::Size, geometry::PixelFormat);
    std::shared_ptr<ClientBuffer> current_buffer();

private:
    std::shared_ptr<ClientBufferFactory> const factory;
    typedef std::map<int, std::shared_ptr<ClientBuffer>> BufferMap;
    BufferMap buffers;
    BufferMap::iterator current_buffer_iter;
    const unsigned max_age;
};
}
}
#endif /* MIR_CLIENT_PRIVATE_MIR_CLIENT_BUFFER_DEPOSITORY_H_ */
