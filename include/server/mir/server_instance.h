/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
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
 *   Alan Griffiths <alan@octopull.co.uk>
 *   Thomas Voss <thomas.voss@canonical.com>
 */

#ifndef MIR_SERVER_INSTANCE_H_
#define MIR_SERVER_INSTANCE_H_

#include <memory>

#include "mir/display_server.h"

namespace mir
{
class ServerConfiguration;

class ServerInstance : public mir::DisplayServer
{
public:
    explicit ServerInstance(ServerConfiguration& config);

    ~ServerInstance();

    void run();
    void stop();

private:
    struct Private;
    std::unique_ptr<Private> p;

    ServerInstance() = delete;
    ServerInstance(const ServerInstance&) = delete;
    ServerInstance& operator=(const ServerInstance&) = delete;
};

}

#endif /* MIR_SERVER_INSTANCE_H_ */
