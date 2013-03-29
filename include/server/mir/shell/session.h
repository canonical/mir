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
 * Authored By: Robert Carr <racarr@canonical.com>
 */

#ifndef MIR_SHELL_SESSION_H_
#define MIR_SHELL_SESSION_H_

#include "mir/frontend/session.h"
#include "mir/input/session_target.h"

#include <mutex>
#include <atomic>
#include <memory>
#include <string>

namespace mir
{

namespace shell
{
class Surface;

class Session : public frontend::Session, public input::SessionTarget
{
public:
    virtual ~Session() {}

    virtual frontend::SurfaceId create_surface(frontend::SurfaceCreationParameters const& params) = 0;
    virtual void destroy_surface(frontend::SurfaceId surface) = 0;
    virtual std::shared_ptr<frontend::Surface> get_surface(frontend::SurfaceId surface) const = 0;

    virtual std::string name() const = 0;
    virtual void shutdown() = 0;

    virtual void hide() = 0;
    virtual void show() = 0;

    virtual std::shared_ptr<Surface> default_surface() const = 0;

protected:
    Session() = default;
    Session(Session const&) = delete;
    Session& operator=(Session const&) = delete;
};

}
}

#endif // MIR_SHELL_SESSION_H_
