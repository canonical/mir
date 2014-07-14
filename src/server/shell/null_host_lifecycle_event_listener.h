/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#ifndef MIR_NULL_HOST_LIFECYCLE_EVENT_LISTENER_H_
#define MIR_NULL_HOST_LIFECYCLE_EVENT_LISTENER_H_

#include "mir/shell/host_lifecycle_event_listener.h"

namespace mir
{
namespace shell
{

class NullHostLifecycleEventListener : public HostLifecycleEventListener
{
public:
    virtual void lifecycle_event_occurred(MirLifecycleState /*state*/) override {}
};

}
}

#endif /* MIR_NULL_HOST_LIFECYCLE_EVENT_LISTENER_H_ */
