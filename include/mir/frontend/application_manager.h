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

#include "mir/frontend/application_session_model.h"
#include "mir/surfaces/application_surface_organiser.h"
#include "mir/frontend/application_focus_strategy.h"
#include "mir/frontend/application_focus_mechanism.h"

#include <memory>
#include <set>

namespace ms = mir::surfaces;
namespace mf = mir::frontend;

namespace mir
{

namespace frontend
{

class ApplicationSessionModel;

class ApplicationManager
{
 public:
    explicit ApplicationManager(std::shared_ptr<ms::ApplicationSurfaceOrganiser> surface_organiser,
                                std::shared_ptr<mf::ApplicationSessionContainer> session_model,
                                std::shared_ptr<mf::ApplicationFocusStrategy> focus_strategy,
                                std::shared_ptr<mf::ApplicationFocusMechanism> focus_mechanism);
    virtual ~ApplicationManager() {}

    std::shared_ptr<ApplicationSession> open_session(std::string name);
    void close_session(std::shared_ptr<ApplicationSession> surface);
    
    void focus_next();

protected:
    ApplicationManager(const ApplicationManager&) = delete;
    ApplicationManager& operator=(const ApplicationManager&) = delete;

private:
    std::shared_ptr<ms::ApplicationSurfaceOrganiser> surface_organiser;
    std::shared_ptr<mf::ApplicationSessionContainer> app_model;
    std::shared_ptr<mf::ApplicationFocusStrategy> focus_strategy;
    std::shared_ptr<mf::ApplicationFocusMechanism> focus_mechanism;

    std::weak_ptr<mf::ApplicationSession> focus_application;
};

}
}

#endif // MIR_FRONTEND_APPLICATION_MANAGER_H_
