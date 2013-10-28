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

#ifndef MIR_DEFAULT_SERVER_STATUS_LISTENER_H_
#define MIR_DEFAULT_SERVER_STATUS_LISTENER_H_

#include "mir/server_status_listener.h"

namespace mir
{
class DefaultServerStatusListener : public virtual ServerStatusListener
{
public:
    virtual void paused()
    {
    }

    virtual void resumed()
    {
    }

    virtual void started()
    {
    }
};
}

#endif /* MIR_DEFAULT_SERVER_STATUS_LISTENER_H_ */
