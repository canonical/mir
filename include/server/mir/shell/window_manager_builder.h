/*
 * Copyright © 2015 Canonical Ltd.
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
 *
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef INCLUDE_SERVER_MIR_SHELL_WINDOW_MANAGER_BUILDER_H_
#define INCLUDE_SERVER_MIR_SHELL_WINDOW_MANAGER_BUILDER_H_

#include <functional>
#include <memory>

namespace mir
{
namespace shell
{
class WindowManager;
class FocusController;

/// WindowManagers are built while initializing an AbstractShell, so a builder functor is needed
using WindowManagerBuilder =
    std::function<std::shared_ptr<WindowManager>(FocusController* focus_controller)>;
}
}
#endif /* INCLUDE_SERVER_MIR_SHELL_WINDOW_MANAGER_BUILDER_H_ */
