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

#ifndef MIR_FRONTEND_APPLICATION_SESSION_H_
#define MIR_FRONTEND_APPLICATION_SESSION_H_

#include <memory>
#include <string>
#include <map>

#include "mir/thread/all.h"

namespace mir
{

namespace surfaces
{

class ApplicationSurfaceOrganiser;
class SurfaceCreationParameters;
class Surface;

}

namespace frontend
{

class ApplicationSession 
{
public:
    explicit ApplicationSession(std::shared_ptr<surfaces::ApplicationSurfaceOrganiser> const& surface_organiser, std::string const& application_name);
    virtual ~ApplicationSession();

    int create_surface(const surfaces::SurfaceCreationParameters& params);
    void destroy_surface(int surface);
    std::shared_ptr<surfaces::Surface> get_surface(int surface) const;

    std::string get_name();

    virtual void hide();
    virtual void show();
protected:
    ApplicationSession(const ApplicationSession&) = delete;
    ApplicationSession& operator=(const ApplicationSession&) = delete;

private:
    std::shared_ptr<surfaces::ApplicationSurfaceOrganiser> const surface_organiser;
    std::string const name;

    int next_id();

    std::atomic<int> next_surface_id;

    typedef std::map<int, std::weak_ptr<surfaces::Surface>> Surfaces;
    Surfaces surfaces;
};

}
}

#endif // MIR_FRONTEND_APPLICATION_SESSION_H_
