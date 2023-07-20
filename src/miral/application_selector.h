/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_APPLICATION_SELECTOR_H
#define MIR_APPLICATION_SELECTOR_H

#include <miral/window_manager_tools.h>
#include <miral/application.h>

namespace miral
{

/// Manages the selection of applications using keyboard shortcuts,
/// most likely via Alt + Tab. When "start" is called, the next application
/// in the SessionContainer's list after the currently focused application
/// will be raised. When "next" is called, the application after that will be
/// raised, and so on. When "complete" is called, the currently raised application
/// will gain focus and be returned.
class ApplicationSelector
{
public:
    ApplicationSelector(WindowManagerTools const&);

    /// Raises the next selectable application in the list for focus selection.
    /// \param reverse If true, the selector will move in reverse through the list of applications
    /// otherwise it will move forward through the list of applications.
    /// \returns The raised application, or nullptr in the case of a failure
    auto next(bool reverse) -> Application;

    /// Focuses the currently selected Application.
    /// \returns The newly selected application, or nullptr in the case of a failure
    auto complete() -> Application;

    /// Cancel the selector if it has been triggered. This will refocus the original application.
    void cancel();

    /// Retrieve whether or not the selector is in progress.
    /// \return true if it is running, otherwise false
    auto is_active() -> bool;

private:
    WindowManagerTools tools;

    /// The application that was focused when "next" was first called
    Application root;

    /// The raised application
    Application selected;

    auto try_select_application(Application) -> bool;
};

}


#endif //MIR_APPLICATION_SELECTOR_H
