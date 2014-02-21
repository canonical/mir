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
 * Authored By: Nick Dedekind <nick.dedekind@canonical.com>
 */

#ifndef MIR_SCENE_SESSION_H_
#define MIR_SCENE_SESSION_H_

#include "mir/shell/session.h"
#include "mir/frontend/session_id.h"

#include <sys/types.h>

namespace mir
{
namespace scene
{
class Session;
class SessionContainer;

class Session : public shell::Session
{
public:
    virtual std::shared_ptr<shell::Session> get_parent() const = 0;
    virtual void set_parent(std::shared_ptr<shell::Session> const& parent) = 0;

    virtual std::shared_ptr<SessionContainer> get_children() const = 0;
};
}
}

#endif // MIR_SCENE_SESSION_H_
