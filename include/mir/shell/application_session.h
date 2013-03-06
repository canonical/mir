/*
 * Copyright © 2013 Canonical Ltd.
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

#ifndef MIR_SHELL_APPLICATION_SESSION_H_
#define MIR_SHELL_APPLICATION_SESSION_H_

#include "mir/shell/session.h"

#include <map>

namespace mir
{

namespace shell
{
class SurfaceFactory;

class ApplicationSession : public Session
{
public:
    explicit ApplicationSession(std::shared_ptr<SurfaceFactory> const& surface_factory, std::string const& session_name);
    ~ApplicationSession();

    SurfaceId create_surface(SurfaceCreationParameters const& params);
    void destroy_surface(SurfaceId surface);
    std::shared_ptr<Surface> get_surface(SurfaceId surface) const;

    std::string name();
    void shutdown();

    void hide();
    void show();

protected:
    ApplicationSession(ApplicationSession const&) = delete;
    ApplicationSession& operator=(ApplicationSession const&) = delete;

private:
    std::shared_ptr<SurfaceFactory> const surface_factory;
    std::string const session_name;

    SurfaceId next_id();

    std::atomic<int> next_surface_id;

    typedef std::map<SurfaceId, std::shared_ptr<Surface>> Surfaces;
    Surfaces::const_iterator checked_find(SurfaceId id) const;
    std::mutex mutable surfaces_mutex;
    Surfaces surfaces;
};

}
} // namespace mir

#endif // MIR_SHELL_APPLICATION_SESSION_H_
