/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_FRONTEND_FOCUS_MECHANISM_H_
#define MIR_FRONTEND_FOCUS_MECHANISM_H_

#include <memory>

namespace mir
{

namespace surfaces
{
class ApplicationSurfaceOrganiser;
}

namespace frontend
{
class ApplicationSession;

class ApplicationFocusMechanism
{
public:
    virtual ~ApplicationFocusMechanism() {}

    virtual void set_focus_to(std::shared_ptr<ApplicationSession> const& new_focus) = 0;

protected:
    ApplicationFocusMechanism() = default;
    ApplicationFocusMechanism(const ApplicationFocusMechanism&) = delete;
    ApplicationFocusMechanism& operator=(const ApplicationFocusMechanism&) = delete;
};

}
}


#endif // MIR_FRONTEND_FOCUS_MECHANISM_H_
