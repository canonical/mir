/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Robert Ancell <robert.ancell@canonical.com>
 */

#ifndef MIR_SERVER_STATUS_H_
#define MIR_SERVER_STATUS_H_

namespace mir
{

class ServerStatus
{
public:
    virtual void paused() = 0;
    virtual void resumed() = 0;

protected:
    ServerStatus() = default;
    virtual ~ServerStatus() = default;
    ServerStatus(ServerStatus const&) = delete;
    ServerStatus& operator=(ServerStatus const&) = delete;
};

}

#endif /* MIR_SERVER_STATUS_H_ */
