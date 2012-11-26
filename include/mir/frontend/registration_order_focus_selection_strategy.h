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

#ifndef MIR_FRONTEND_REGISTRATION_FOCUS_STRATEGY_H_
#define MIR_FRONTEND_REGISTRATION_FOCUS_STRATEGY_H_

#include "mir/frontend/application_focus_selection_strategy.h"

namespace mir
{
namespace frontend
{
class SessionContainer;

class RegistrationOrderFocusSelectionStrategy : public ApplicationFocusSelectionStrategy
{
public:
    explicit RegistrationOrderFocusSelectionStrategy(std::shared_ptr<SessionContainer> const& app_container);
    virtual ~RegistrationOrderFocusSelectionStrategy() {}

    virtual std::weak_ptr<Session> next_focus_app(std::shared_ptr<Session> const& focused_app);
    virtual std::weak_ptr<Session> previous_focus_app(std::shared_ptr<Session> const& focused_app);

protected:
    RegistrationOrderFocusSelectionStrategy(const RegistrationOrderFocusSelectionStrategy&) = delete;
    RegistrationOrderFocusSelectionStrategy& operator=(const RegistrationOrderFocusSelectionStrategy&) = delete;
private:
    std::shared_ptr<SessionContainer> app_container;
};

}
}


#endif // MIR_FRONTEND_FOCUS_STRATEGY_H_
