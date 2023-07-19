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
/// in the SessionContainer's list after the currently focuse application
/// will be raised. When "next" is called, the application after that will be
/// raised, and so on. When "complete" is called, the currently raised application
/// will gain focus and be returned.
class ApplicationSelector
{
public:
    ApplicationSelector(WindowManagerTools const&);

    /// Begins application selection by raising the application
    /// that immediately follows the currently selected application
    /// in the SessionContainer's list.
    /// \param reverse If true, selection will happen in reverse, otherwise forward.
    void start(bool reverse);

    /// Lowers the currently raised application and raises the application
    /// that follows it.
    /// \returns The raised application
    auto next() -> Application ;

    /// Focuses the currently selected Application.
    /// \returns The newly selected application
    auto complete() -> Application ;

    /// Retrieve whether or not the selector is in progress.
    /// \return true if it is running, otherwise false
    auto is_active() -> bool;

private:
    WindowManagerTools tools;
    bool reverse = false;
    bool is_started = false;

    /// The raised application
    ApplicationInfo selected;

    /// Figure out the next application in the list and then raise it.
    void raise_next();
};

}


#endif //MIR_APPLICATION_SELECTOR_H
