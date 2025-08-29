/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_SHELL_APPLICATION_SWITCHER_H_
#define MIR_SHELL_APPLICATION_SWITCHER_H_

#include "mir/shell/application_switcher.h"

namespace mir
{
namespace shell
{
class BasicApplicationSwitcher : public ApplicationSwitcher
{
public:
    explicit BasicApplicationSwitcher(Executor& executor);
    ~BasicApplicationSwitcher() override = default;

    void activate(ApplicationSwitcherState state) override;
    void deactivate() override;
    void set_state(ApplicationSwitcherState state) override;
    void next() override;
};
}
}

#endif // MIR_SHELL_APPLICATION_SWITCHER_H_