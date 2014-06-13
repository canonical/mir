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
 * \note All functions are non-blocking
 */
class Transport
{
public:
    virtual ~Transport() = default;

    /**
     * @brief The Observer class
     */
    class Observer
    {
    public:
        virtual ~Observer() = default;
        virtual void on_data_available() = 0;
        virtual void on_disconnected() = 0;
    };

    /**
     * @brief register_observer
     * @param observer
     */
    virtual void register_observer(std::shared_ptr<Observer> const& observer) = 0;

    /**
     * @brief receive_data
     * @param buffer
     * @param message_size
     * @return
     */
    virtual size_t receive_data(void* buffer, size_t message_size) = 0;
    /**
     * @brief receive_file_descriptors
     * @param fds
     */
    virtual void receive_file_descriptors(std::vector<int> &fds) = 0;

    /**
     * @brief send_message
     * @param message
     * @return
     */
    virtual size_t send_data(std::vector<uint8_t> const& buffer) = 0;
};

}
}
}

#endif // MIR_CLIENT_RPC_TRANSPORT_H
