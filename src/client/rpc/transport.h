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


#ifndef MIR_CLIENT_RPC_TRANSPORT_H
#define MIR_CLIENT_RPC_TRANSPORT_H

#include <functional>
#include <vector>
#include <memory>
#include <stdint.h>

namespace mir
{
namespace client
{
namespace rpc
{
/**
 * \brief Responsible for shuttling bytes to and from the server
 *
 * \note It is safe to call a read and a write method simultaneously
 *       from different threads. Multiple threads calling the same
 *       function need synchronisation.
 */
class StreamTransport
{
public:
    virtual ~StreamTransport() = default;

    /**
     * \brief Observer of IO status
     * \note The Transport may call Observer members from arbitrary threads.
     *       The Observer implementation is responsible for any synchronisation.
     */
    class Observer
    {
    public:
        virtual ~Observer() = default;
        /**
         * \brief Called by the Transport when data is available for reading
         */
        virtual void on_data_available() = 0;
        /**
         * \brief Called by the Transport when the connection to the server has been broken.
         */
        virtual void on_disconnected() = 0;
    };

    /**
     * \brief Register an IO observer
     * \param [in] observer
     * \note There is no guarantee which thread will call into the observer.
     *       Synchronisation is the responsibility of the caller.
     */
    virtual void register_observer(std::shared_ptr<Observer> const& observer) = 0;

    /**
     * \brief Read data from the server
     * \param [out] buffer          Buffer to read into
     * \param [in]  read_bytes      Number of bytes to read
     * \throws A std::runtime_error if it is not possible to read
     *         read_bytes bytes from the server.
     *
     * \note This provides stream semantics - message boundaries are not preserved.
     */
    virtual void receive_data(void* buffer, size_t read_bytes) = 0;

    /**
     * \brief Receive file descriptors from the server
     * \param [in,out] fds  Vector to populate with received file descriptors.
     *                      The size of this vector determines how many descriptors
     *                      are attempted to be received.
     * \throws A std::runtime_error if it is not possible to read fds.size() file
     *         descriptors from the server.
     */
    virtual void receive_file_descriptors(std::vector<int> &fds) = 0;

    /**
     * \brief Write data to the server
     * \param [in] buffer   Data to send
     * \throws A std::runtime_error if it is not possible to write the full contents
     *         of buffer to the server.
     */
    virtual void send_data(std::vector<uint8_t> const& buffer) = 0;
};

}
}
}

#endif // MIR_CLIENT_RPC_TRANSPORT_H
