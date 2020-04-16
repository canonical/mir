/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#ifndef MIR_HOST_LIFECYCLE_EVENT_LISTENER_H_
#define MIR_HOST_LIFECYCLE_EVENT_LISTENER_H_

#include "mir_toolkit/common.h"

namespace mir
{
namespace shell
{

class HostLifecycleEventListener
{
public:
    virtual void lifecycle_event_occurred(MirLifecycleState state) = 0;

protected:
    HostLifecycleEventListener() = default;
    virtual ~HostLifecycleEventListener() = default;
    HostLifecycleEventListener(HostLifecycleEventListener const&) = delete;
    HostLifecycleEventListener& operator=(HostLifecycleEventListener const&) = delete;
};

}
}

#endif /* MIR_HOST_LIFECYCLE_EVENT_LISTENER_H_ */
