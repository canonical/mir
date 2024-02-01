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

/// Intended to be used in conjunction with a WindowManagementPolicy. Simply
/// hook up the "advise" methods to be called at the appropriate times, and
/// the ApplicationSelector will keep track of the rest.
/// Manages the selection of applications using keyboard shortcuts,
/// most likely via Alt + Tab. When "start" is called, the next application
/// in the SessionContainer's list after the currently focused application
/// will be raised. When "next" is called, the application after that will be
/// raised, and so on. When "complete" is called, the currently raised application
/// will gain focus and be returned.
/// \remark Since MirAL 3.10
class ApplicationSelector
{
public:
    explicit ApplicationSelector(WindowManagerTools const&);
    ~ApplicationSelector();
    ApplicationSelector(ApplicationSelector const&) = delete;
    auto operator=(ApplicationSelector const&) -> ApplicationSelector& = delete;

    /// Called when a new application has been added to the list.
    void advise_new_app(Application const&);

    /// Called when focus is given to a window.
    void advise_focus_gained(WindowInfo const&);

    /// Called when focus is lost on a window.
    void advise_focus_lost(WindowInfo const&);

    /// Called when an application is deleted.
    void advise_delete_app(Application const&);

    /// Raises the next selectable application in the list for focus selection.
    /// \returns The raised application, or nullptr in the case of a failure
    auto next() -> Application;

    /// Raises the previous selectable application in the list for focus selection.
    /// \returns The raised application or nullptr in the case of a failure
    auto prev() -> Application ;

    /// Focuses the currently selected Application.
    /// \returns The newly selected application, or nullptr in the case of a failure
    auto complete() -> Application;

    /// Cancel the selector if it has been triggered. This will refocus the original application.
    void cancel();

    /// Retrieve whether or not the selector is in progress.
    /// \return true if it is running, otherwise false
    auto is_active() -> bool;

    /// Retrieve the focused application.
    /// \returns The focused application
    auto get_focused() -> Application;

  private:
    struct Impl;
    std::unique_ptr<Impl> self;
};

}


#endif //MIR_APPLICATION_SELECTOR_H
