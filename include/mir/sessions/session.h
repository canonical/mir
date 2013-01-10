/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored By: Robert Carr <racarr@canonical.com>
 */

#ifndef MIR_SESSIONS_SESSION_H_
#define MIR_SESSIONS_SESSION_H_

#include "mir/int_wrapper.h"
#include "mir/thread/all.h"

#include <memory>
#include <string>
#include <map>

namespace mir
{

/// Management of client application sessions
namespace sessions
{
class Surface;
class SurfaceFactory;
class SurfaceCreationParameters;
typedef IntWrapper<IntWrapperTypeTag::SessionsSurfaceId> SurfaceId;

class Session
{
public:
    explicit Session(std::shared_ptr<SurfaceFactory> const& surface_factory, std::string const& session_name);
    virtual ~Session();

    SurfaceId create_surface(const SurfaceCreationParameters& params);
    void destroy_surface(SurfaceId surface);
    std::shared_ptr<Surface> get_surface(SurfaceId surface) const;

    std::string name();
    void shutdown();

    virtual void hide();
    virtual void show();
protected:
    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;

private:
    std::shared_ptr<SurfaceFactory> const surface_factory;
    std::string const session_name;

    SurfaceId next_id();

    std::atomic<int> next_surface_id;

    typedef std::map<SurfaceId, std::shared_ptr<Surface>> Surfaces;
    std::mutex mutable surfaces_mutex;
    Surfaces surfaces;
};

}
}

#endif // MIR_SESSIONS_SESSION_H_
