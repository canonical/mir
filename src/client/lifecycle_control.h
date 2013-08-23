/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Ricardo Mendoza <ricardo.mendoza@canonical.com>
 */

#ifndef MIR_LIFECYCLE_CONTROL_H_
#define MIR_LIFECYCLE_CONTROL_H_

#include "mir_toolkit/common.h"

#include <functional>
#include <mutex>

namespace mir
{
namespace client
{
class LifecycleControl
{
public:
    LifecycleControl();
    ~LifecycleControl();

    void set_lifecycle_event_handler(std::function<void(MirLifecycleState)> const&);
    void call_lifecycle_event_handler(uint32_t state);

private:
    std::mutex mutable guard;
    std::function<void(MirLifecycleState)> handle_lifecycle_event;
};
}
}

#endif /* MIR_LIFECYCLE_CONTROL_H_ */
