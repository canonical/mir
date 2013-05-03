/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_EXAMPLES_APPLICATION_SWITCHER_H_
#define MIR_EXAMPLES_APPLICATION_SWITCHER_H_

#include "mir/input/event_filter.h"
#include "mir/geometry/displacement.h"

#include <memory>

namespace mir
{
namespace shell
{
class FocusController;
class SessionManager;
}
namespace examples
{

class ApplicationSwitcher : public input::EventFilter
{
public: 
    ApplicationSwitcher();
    ~ApplicationSwitcher() = default;

    void set_focus_controller(std::shared_ptr<shell::FocusController> const& focus_controller);
    void set_session_manager(std::shared_ptr<shell::SessionManager> const& sm);
    
    bool handles(MirEvent const& event);

protected:
    ApplicationSwitcher(const ApplicationSwitcher&) = delete;
    ApplicationSwitcher& operator=(const ApplicationSwitcher&) = delete;

private:
    std::shared_ptr<shell::FocusController> focus_controller;
    std::shared_ptr<shell::SessionManager> session_manager;
    geometry::Displacement click;
};

}
} // namespace mir

#endif // MIR_EXAMPLES_APPLICATION_SWITCHER_H_
