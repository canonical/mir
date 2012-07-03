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
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#ifndef MIR_FRONTEND_APPLICATION_MANAGER_H_
#define MIR_FRONTEND_APPLICATION_MANAGER_H_

#include "mir/frontend/application.h"
#include "mir/frontend/services/input_grab_controller.h"
#include "mir/frontend/services/surface_factory.h"

#include <memory>

namespace mir
{

namespace surfaces
{

class SurfaceController;

}

namespace frontend
{
    
namespace ms = mir::surfaces;

class ApplicationManager
        : public frontend::services::InputGrabController,
          public frontend::services::SurfaceFactory
{
 public:
    explicit ApplicationManager(ms::SurfaceController* surface_controller);
    virtual ~ApplicationManager() {}

    // From InputGrabController
    void grab_input_for_application(std::weak_ptr<Application> app);
    std::weak_ptr<Application> get_grabbing_application();
    void release_grab();

    // From SurfaceFactory
    std::shared_ptr<ms::Surface> create_surface();
    void destroy_surface(std::shared_ptr<ms::Surface> surface);
    
  protected:
    ApplicationManager(const ApplicationManager&) = delete;
    ApplicationManager& operator=(const ApplicationManager&) = delete;

  private:
    std::weak_ptr<Application> grabbing_application;
    ms::SurfaceController* surface_controller;
};

}
}

#endif // MIR_FRONTEND_APPLICATION_MANAGER_H_
