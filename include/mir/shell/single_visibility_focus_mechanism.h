/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_SHELL_SINGLE_VISIBILITY_FOCUS_MECHANISM_H_
#define MIR_SHELL_SINGLE_VISIBILITY_FOCUS_MECHANISM_H_

#include <memory>
#include "mir/shell/focus_setter.h"

namespace mir
{

namespace shell
{
class Session;
class SessionContainer;

class SingleVisibilityFocusMechanism : public FocusSetter
{
public:
    explicit SingleVisibilityFocusMechanism(std::shared_ptr<SessionContainer> const& app_container);
    virtual ~SingleVisibilityFocusMechanism() {}

    void set_focus_to(std::shared_ptr<Session> const& new_focus);

protected:
    SingleVisibilityFocusMechanism(const SingleVisibilityFocusMechanism&) = delete;
    SingleVisibilityFocusMechanism& operator=(const SingleVisibilityFocusMechanism&) = delete;
private:
    std::shared_ptr<SessionContainer> app_container;
};

}
}


#endif // MIR_SHELL_SINGLE_VISIBILITY_FOCUS_MECHANISM_H_
