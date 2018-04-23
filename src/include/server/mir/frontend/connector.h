/*
 * Copyright © 2012-2018 Canonical Ltd.
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
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#ifndef MIR_FRONTEND_CONNECTOR_H_
#define MIR_FRONTEND_CONNECTOR_H_

#include <mir/optional_value.h>
#include <functional>
#include <memory>

namespace mir
{
namespace frontend
{
class Session;

/// Handle client process connections
class Connector
{
public:
    virtual void start() = 0;
    virtual void stop() = 0;

    virtual int client_socket_fd() const = 0;

    virtual int client_socket_fd(std::function<void(std::shared_ptr<Session> const& session)> const& connect_handler) const = 0;

    virtual auto socket_name() const -> optional_value<std::string> = 0;

protected:
    Connector() = default;
    virtual ~Connector() = default;
    Connector(const Connector&) = delete;
    Connector& operator=(const Connector&) = delete;
};
}
}

#endif // MIR_FRONTEND_CONNECTOR_H_
