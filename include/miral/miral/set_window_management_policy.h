/*
 * Copyright © 2016 Canonical Ltd.
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
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIRAL_SET_WINDOW_MANAGEMENT_POLICY_H
#define MIRAL_SET_WINDOW_MANAGEMENT_POLICY_H

#include <functional>
#include <memory>

namespace mir
{
class Server;
}

namespace miral
{
class WindowManagerTools;
class WindowManagementPolicy;

class SetWindowManagementPolicy
{
public:
    SetWindowManagementPolicy(std::function<std::unique_ptr<WindowManagementPolicy>(WindowManagerTools const& tools)> const& builder);
    ~SetWindowManagementPolicy();

    void operator()(mir::Server& server) const;

private:
    std::function<std::unique_ptr<WindowManagementPolicy>(WindowManagerTools const& tools)> builder;
};

template<typename Policy, typename ...Args>
auto set_window_management_policy(Args& ... args) -> SetWindowManagementPolicy
{
    return SetWindowManagementPolicy{[&args...](WindowManagerTools const& tools) -> std::unique_ptr<WindowManagementPolicy>
        { return std::make_unique<Policy>(tools, args...); }};
}
}

#endif //MIRAL_SET_WINDOW_MANAGEMENT_POLICY_H
