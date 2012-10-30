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

#ifndef MIR_FRONTEND_FOCUS_STRATEGY_H_
#define MIR_FRONTEND_FOCUS_STRATEGY_H_

#include <memory>

namespace mir
{
namespace frontend
{
class ApplicationSession;

class ApplicationFocusStrategy
{
public:
    virtual ~ApplicationFocusStrategy() {}

    virtual std::weak_ptr<ApplicationSession> next_focus_app (std::shared_ptr<ApplicationSession> focused_app) = 0;
    std::weak_ptr<ApplicationSession> previous_focus_app (std::shared_ptr<ApplicationSession> focused_app);

protected:
    ApplicationFocusStrategy() = default;
    ApplicationFocusStrategy(const ApplicationFocusStrategy&) = delete;
    ApplicationFocusStrategy& operator=(const ApplicationFocusStrategy&) = delete;
};

}
}


#endif // MIR_FRONTEND_FOCUS_STRATEGY_H_
