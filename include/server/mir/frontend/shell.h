/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#ifndef MIR_FRONTEND_SHELL_H_
#define MIR_FRONTEND_SHELL_H_

#include "mir/frontend/surface_id.h"
#include <memory>

namespace mir
{
namespace shell
{
struct SurfaceCreationParameters;
}
namespace frontend
{
class EventSink;
class Session;

class Shell
{
public:
    virtual ~Shell() {}

    virtual std::shared_ptr<Session> open_session(
        std::string const& name, std::shared_ptr<EventSink> const& sink) = 0;
    virtual void close_session(std::shared_ptr<Session> const& session)  = 0;

    virtual SurfaceId create_surface_for(std::shared_ptr<Session> const& session,
                                         shell::SurfaceCreationParameters const& params) = 0;

protected:
    Shell() = default;
    Shell(const Shell&) = delete;
    Shell& operator=(const Shell&) = delete;
};

}
}

#endif // MIR_FRONTEND_SHELL_H_
