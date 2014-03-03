/*
 * Copyright © 2012-2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.   See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.   If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored By: Robert Carr <racarr@canonical.com>
 */

#ifndef MIR_SHELL_SESSION_H_
#define MIR_SHELL_SESSION_H_

#include "mir/frontend/session.h"
#include "mir/shell/snapshot.h"

#include <sys/types.h>

namespace mir
{
namespace shell
{
class Surface;

class Session : public frontend::Session
{
public:
    virtual void force_requests_to_complete() = 0;
    virtual pid_t process_id() const = 0;

    virtual void take_snapshot(SnapshotCallback const& snapshot_taken) = 0;
    virtual std::shared_ptr<Surface> default_surface() const = 0;
    virtual void set_lifecycle_state(MirLifecycleState state) = 0;
};
}
}

#endif // MIR_SHELL_SESSION_H_
