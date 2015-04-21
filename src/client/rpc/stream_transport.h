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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */


#ifndef MIR_CLIENT_RPC_STREAM_TRANSPORT_H_
#define MIR_CLIENT_RPC_STREAM_TRANSPORT_H_

#include <vector>
#include <memory>
#include <stdint.h>

#include "mir/fd.h"
#include "mir/dispatch/dispatchable.h"

namespace mir
{
namespace client
{

/**
 * \brief Client/Server communication implementation namespace.
 *
 * \par Theory of operation:
 * The RPC layer is built in two parts: the transport layer and the protocol layer.
 *
 * The transport layer handles moving bytes and file descriptors between
 * the client and the server, notifying when data is available, and notifying
 * when the link has been disconnected..
 *
 * The protocol layer is responsible for mediating between the rest of the code
 * and the transport layer. It provides an RPC interface built upon the transport.
 */
namespace rpc
{
/**
 * \brief Responsible for shuttling bytes to and from the server
 *
 * This is a transport providing stream semantics. It does not preserve message
 * boundaries, writes from the remote end are not guaranteed to become available
 * for reading atomically, and local writes are not guaranteed to become available
 * for reading at the remote end atomically.
 *
 * In practice reads and writes of “small size” will be atomic. See your kernel
 * source tree for a definition of “small size” :).
 *
 * As it does not preserve message boundaries, partial reads do not discard any
 * unread data waiting for reading. This applies for both binary data and file descriptors.
 *
 * \note It is safe to call a read and a write method simultaneously
 *       from different threads. Multiple threads calling the same
 *       function need synchronisation.
 */
class StreamTransport : public dispatch::Dispatchable
{
public:
    /**
     * \note Upon completion of the destructor it is guaranteed that no methods will be called
     *       by the StreamTransport on any Observer that had been registered.
     */
    virtual ~StreamTransport() = default;

    /* Delete CopyAssign */
    StreamTransport() = default;
    StreamTransport(StreamTransport const&) = delete;
    StreamTransport& operator=(StreamTransport const&) = delete;

    /**
     * \brief Observer of IO status
     * \note The Transport will only call Observers in response to dispatch(),
     *       and on the thread calling dispatch().
     */
    class Observer
    {
    public:
        Observer() = default;
        virtual ~Observer() = default;
        /**
         * \brief Called by the Transport when data is available for reading
         */
        virtual void on_data_available() = 0;
        /**
         * \brief Called by the Transport when the connection to the server has been broken.
         * \note This is not guaranteed to be triggered exactly once; it may not fire
         *       during destruction of the Transport, or it may fire multiple times.
         */
        virtual void on_disconnected() = 0;

        Observer(Observer const&) = delete;
        Observer& operator=(Observer const&) = delete;
    };

    /**
     * \brief Register an IO observer
     * \param [in] observer
     */
    virtual void register_observer(std::shared_ptr<Observer> const& observer) = 0;

    /**
     * \brief Unregister a previously-registered observer.
     * \param [in] observer. This object must be managed by one of the shared_ptrs previously
     *             registered via register_observer().
     */
    virtual void unregister_observer(std::shared_ptr<Observer> const& observer) = 0;

    /**
     * \brief Read data from the server
     * \param [out] buffer          Buffer to read into
     * \param [in]  bytes_requested Number of bytes to read
     * \throws A std::runtime_error if it is not possible to read
     *         read_bytes bytes from the server.
     *
     * \note This provides stream semantics - message boundaries are not preserved.
     */
    virtual void receive_data(void* buffer, size_t bytes_requested) = 0;

    /**
     * \brief Read data and file descriptors from the server
     * \param [out] buffer          Buffer to read into
     * \param [in]  bytes_requested Number of bytes to read
     * \param [in,out] fds          File descriptors received in this read.
     *                              The value of fds.size() determines the number of
     *                              file descriptors to receive.
     * \throws A std::runtime_error if it is not possible to read
     *         read_bytes bytes from the server or if it is not possible to read
     *         fds.size() file descriptors from the server.
     *
     * \note This provides stream semantics - message boundaries are not preserved.
     */
    virtual void receive_data(void* buffer, size_t bytes_requested, std::vector<Fd>& fds) = 0;

    /**
     * \brief Write message to the server
     * \param [in] buffer   Data to send
     * \param [in] fds      Fds to send
     * \throws A std::runtime_error if it is not possible to write the full contents
     *         of buffer to the server.
     */
    virtual void send_message(std::vector<uint8_t> const& buffer, std::vector<Fd> const& fds) = 0;
};

}
}
}

#endif // MIR_CLIENT_RPC_STREAM_TRANSPORT_H_
