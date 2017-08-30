/*
 * Copyright © 2014-2016 Canonical Ltd.
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
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIRAL_WINDOW_MANAGEMENT_OPTIONS_H_
#define MIRAL_WINDOW_MANAGEMENT_OPTIONS_H_

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace mir
{
class Server;
}

namespace miral
{
class WindowManagerTools;
class WindowManagementPolicy;

using WindowManagementPolicyBuilder =
    std::function<std::unique_ptr<miral::WindowManagementPolicy>(WindowManagerTools const& tools)>;

struct WindowManagerOption
{
    std::string const name;
    WindowManagementPolicyBuilder const build;
};

template<typename Policy, typename ...Args>
inline auto add_window_manager_policy(std::string const& name, Args&... args) -> WindowManagerOption
{
    return {name, [&args...](WindowManagerTools const& tools) -> std::unique_ptr<miral::WindowManagementPolicy>
        { return std::make_unique<Policy>(tools, args...); }};
}

class WindowManagerOptions
{
public:
    WindowManagerOptions() = default;
    explicit WindowManagerOptions(std::initializer_list<WindowManagerOption> const& policies) : policies(policies) {}

    void operator()(mir::Server& server) const;

    std::vector<WindowManagerOption> const policies;
};
}

#endif // MIRAL_WINDOW_MANAGEMENT_OPTIONS_H_
