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

#ifndef MIR_FRONTEND_SINGLE_VISIBILITY_FOCUS_MECHANISM_H_
#define MIR_FRONTEND_SINGLE_VISIBILITY_FOCUS_MECHANISM_H_

#include <memory>
#include "mir/frontend/application_focus_mechanism.h"

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

class SingleVisibilityFocusMechanism : public ApplicationFocusMechanism
{
public:
    explicit SingleVisibilityFocusMechanism() {}
    virtual ~SingleVisibilityFocusMechanism() {}

    void focus (std::shared_ptr<ApplicationSessionContainer> model, 
                std::shared_ptr<ApplicationSession> new_focus);

protected:
    SingleVisibilityFocusMechanism(const SingleVisibilityFocusMechanism&) = delete;
    SingleVisibilityFocusMechanism& operator=(const SingleVisibilityFocusMechanism&) = delete;
};

}
}


#endif // MIR_FRONTEND_SINGLE_VISIBILITY_FOCUS_MECHANISM_H_
