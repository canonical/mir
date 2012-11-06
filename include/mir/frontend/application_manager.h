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

#include <memory>
#include "mir/frontend/application_session_factory.h"

namespace mir
{

namespace surfaces
{
class ApplicationSurfaceOrganiser;
}

namespace frontend
{

class ApplicationSession;
class ApplicationSessionContainer;
class ApplicationFocusSelectionStrategy;
class ApplicationFocusMechanism;

class ApplicationManager : public ApplicationSessionFactory
{
 public:
    explicit ApplicationManager(std::shared_ptr<surfaces::ApplicationSurfaceOrganiser> const& surface_organiser,
                                std::shared_ptr<ApplicationSessionContainer> const& session_container,
                                std::shared_ptr<ApplicationFocusSelectionStrategy> const& focus_selection_strategy,
                                std::shared_ptr<ApplicationFocusMechanism> const& focus_mechanism);
    virtual ~ApplicationManager() {}

    virtual std::shared_ptr<ApplicationSession> open_session(std::string name);
    virtual void close_session(std::shared_ptr<ApplicationSession> const& session);
    
    void focus_next();

protected:
    ApplicationManager(const ApplicationManager&) = delete;
    ApplicationManager& operator=(const ApplicationManager&) = delete;

private:
    std::shared_ptr<surfaces::ApplicationSurfaceOrganiser> surface_organiser;
    std::shared_ptr<ApplicationSessionContainer> app_container;
    std::shared_ptr<ApplicationFocusSelectionStrategy> focus_selection_strategy;
    std::shared_ptr<ApplicationFocusMechanism> focus_mechanism;

    std::weak_ptr<ApplicationSession> focus_application;
};

}
}

#endif // MIR_FRONTEND_APPLICATION_MANAGER_H_
