/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIRAL_APPLICATION_SWITCHER_H
#define MIRAL_APPLICATION_SWITCHER_H

#include <memory>
#include <mir/shell/application_switcher.h>

namespace mir
{
class Server;
}

namespace miral
{
class ApplicationSwitcher
{
public:
    ApplicationSwitcher();
    void operator()(mir::Server& server) const;
    void activate(mir::shell::ApplicationSwitcherState state) const;
    void deactivate() const;
    void set_state(mir::shell::ApplicationSwitcherState state) const;
    void next() const;

private:
    struct Self;
    std::shared_ptr<Self> self;
};
}

#endif // MIRAL_APPLICATION_SWITCHER_H