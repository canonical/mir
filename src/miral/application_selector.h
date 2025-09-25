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
class ApplicationSelector
{
public:
    explicit ApplicationSelector(WindowManagerTools const&);
    ~ApplicationSelector();
    ApplicationSelector(ApplicationSelector const&);
    auto operator=(ApplicationSelector const&) -> ApplicationSelector&;

    /// Called when a window is created
    void advise_new_window(WindowInfo const&);

    void advise_new_window(WindowInfo const&, bool focused);

    /// Called when focus is given to a window.
    void advise_focus_gained(WindowInfo const&);

    /// Called when focus is lost on a window.
    void advise_focus_lost(WindowInfo const&);

    /// Called when a window is deleted
    void advise_delete_window(WindowInfo const&);

    /// Raises the next selectable application in the list for focus selection.
    /// \returns The raised window
    auto next(bool within_app) -> Window;

    /// Raises the previous selectable application in the list for focus selection.
    /// \returns The raised window
    auto prev(bool within_app) -> Window;

    /// Focuses the currently selected Application.
    /// \returns The raised window
    auto complete() -> Window;

    /// Cancel the selector if it has been triggered. This will refocus the original application.
    void cancel();

    /// Retrieve whether or not the selector is in progress.
    /// \return true if it is running, otherwise false
    auto is_active() const -> bool;

    /// Retrieve the focused application.
    /// \returns The focused application
    auto get_focused() -> Window;

private:
    auto advance(bool reverse, bool within_app) -> Window;
    auto find(Window) -> std::vector<Window>::iterator;
    void select(Window const&);

    WindowManagerTools tools;

    /// Represents the current order of focus by application. Most recently focused
    /// applications are at the beginning, while least recently focused are at the end.
    std::vector<Window> focus_list;

    /// The application that was selected when next was first called
    std::vector<Window>::iterator originally_selected_it;

    /// The application that is currently selected.
    Window selected;
    std::optional<MirWindowState> restore_state;

    bool is_active_ = false;
};

}


#endif //MIR_APPLICATION_SELECTOR_H
