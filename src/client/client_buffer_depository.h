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
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *   Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_CLIENT_PRIVATE_MIR_CLIENT_BUFFER_DEPOSITORY_H_
#define MIR_CLIENT_PRIVATE_MIR_CLIENT_BUFFER_DEPOSITORY_H_

#include <list>
#include <memory>

#include "mir_toolkit/common.h"
#include "mir/geometry/size.h"

struct MirBufferPackage;

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
/// \sa mir::frontend::ClientBufferTracker for the server-side analogue.
/// \note Changes to the tracking algorithm will need associated server-side changes to mir::frontend::ClientBufferTracker
class ClientBufferDepository
{
public:
    /// Constructor

    /// \param factory is the ClientBufferFactory that will be used to convert IPC data to ClientBuffers
    /// \param max_buffers is the number of buffers the depository will cache. After the client has received
    /// its initial buffers the ClientBufferDepository will always have the last max_buffers buffers
    /// cached.
    /// \note The server relies on the depository caching max_buffers buffers to optimise away buffer
    /// passing. As such, this number needs to be shared between the server and client libraries.
    ClientBufferDepository(std::shared_ptr<ClientBufferFactory> const& factory, int max_buffers);

    /// Construct a ClientBuffer from the IPC data, and use it as the current buffer.

    /// This also marks the previous current buffer (if any) as being submitted to the server.
    /// \post current_buffer() will return a ClientBuffer constructed from this IPC data.
    void deposit_package(std::shared_ptr<MirBufferPackage> const&, int id,
                         geometry::Size, MirPixelFormat);
    std::shared_ptr<ClientBuffer> current_buffer();
    uint32_t current_buffer_id() const;
    void set_max_buffers(unsigned int max_buffers);
private:
    std::shared_ptr<ClientBufferFactory> const factory;
    std::list<std::pair<int, std::shared_ptr<ClientBuffer>>> buffers;
    unsigned int max_buffers;
};
}
}
#endif /* MIR_CLIENT_PRIVATE_MIR_CLIENT_BUFFER_DEPOSITORY_H_ */
