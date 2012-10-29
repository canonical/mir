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

#include "mir/frontend/services/surface_factory.h"

#include <memory>
#include <string>
#include <vector>

namespace mf = mir::frontend;

namespace mir
{

namespace surfaces
{

class ApplicationSurfaceOrganiser;
class SurfaceCreationParameters;

}

namespace frontend
{

namespace ms = mir::surfaces;

namespace detail
{
    class Session;
}

namespace mfd = mir::frontend::detail;

class ApplicationSession : public frontend::services::SurfaceFactory
{
 public:
    explicit ApplicationSession(ms::ApplicationSurfaceOrganiser* surface_organiser, std::weak_ptr<mfd::Session> session);
    virtual ~ApplicationSession() {}

    // From SurfaceFactory
    std::weak_ptr<ms::Surface> create_surface(const ms::SurfaceCreationParameters& params);
    void destroy_surface(std::weak_ptr<ms::Surface> surface);
    
  protected:
    ApplicationSession(const ApplicationSession&) = delete;
    ApplicationSession& operator=(const ApplicationSession&) = delete;

  private:
    std::weak_ptr<mfd::Session> session;
    std::vector<std::weak_ptr<ms::Surface>> surfaces;
    ms::ApplicationSurfaceOrganiser* surface_organiser;
};

}
}

#endif // MIR_FRONTEND_APPLICATION_SESSION_H_
