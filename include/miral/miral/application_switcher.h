/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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

#ifndef MIRAL_APPLICATION_SWITCHER_H
#define MIRAL_APPLICATION_SWITCHER_H

#include <miral/window_manager_tools.h>
#include <memory>
#include <wayland-client.h>
#include <functional>
#include <miral/toolkit_event.h>
#include <linux/input-event-codes.h>

namespace miral
{

/// A simple application switcher.
///
/// This class spawns an internal wayland client. Shells can bind the methods
/// defined in this class to custom keybinds. For example, a shell may bind `alt + tab`
/// to trigger #ApplicationSwitcher::next_app.
///
/// Instances of this class should be provided to #miral::StartupInternalClient.
///
/// Users must call #ApplicationSwitcher::stop in the stop callback of their server
/// to stop the switcher.
///
/// This internal client relies on the following protocols:
/// 1. wlr foreign toplevel management v1
/// 2. wlr layer shell v1
///
/// \warning This is implementation is a work in progress. As such, it may be
///          lacking features that are expected from a typical application switcher
///          client. Use at your own risk.
///
/// \remark Since MirAL 5.6
class ApplicationSwitcher
{
public:
    /// Creates a new application switcher.
    ApplicationSwitcher();
    ~ApplicationSwitcher();

    void operator()(wl_display* display);
    void operator()(std::weak_ptr<mir::scene::Session> const& session) const;

    /// Tentatively select the next application in the focus list.
    ///
    /// The application will not be focused until #confirm is called.
    void next_app() const;

    /// Tentatively select the previous application in the focus list.
    ///
    /// The application will not be focused until #confirm is called.
    void prev_app() const;

    /// Tentatively select the next window within the tentatively focused
    /// application.
    ///
    /// The window will not be focused until #confirm is called.
    void next_window() const;

    /// Tentatively select the previous window within the tentatively focused
    /// application.
    ///
    /// The window will not be focused until #confirm is called.
    void prev_window() const;

    /// Focus the tentatively selected application and hide it.
    void confirm() const;

    /// Cancel application switching.
    void cancel() const;

    /// Stop the application switcher altogether.
    void stop() const;

private:
    class Self;
    std::shared_ptr<Self> self;
};

/// This class provides a common shortcut strategy for the
/// #miral::ApplicationSwitcher.
///
/// By default, this class will do the following:
/// - Call #miral::ApplicationSwitcher::next_app on `alt + tab`.
/// - Call #miral::ApplicationSwitcher::prev_app on `alt + shift + tab`
/// - Call #miral::ApplicationSwitcher::next_window on `alt + grave`
/// - Call #miral::ApplicationSwitcher::prev_window` on `alt + shift + grave`.
///
/// The default shortcuts may be changed by the shell author.
///
/// This class should be provided to a #miral::AppendKeyboardEventFilter.
///
/// \remark Since MirAL 5.6
class ApplicationSwitcherKeyboardFilter
{
public:
    explicit ApplicationSwitcherKeyboardFilter(ApplicationSwitcher const& switcher);
    ~ApplicationSwitcherKeyboardFilter();

    /// Sets the primary modifier for the keyboard filter.
    ///
    /// This is the modifier that must be held for any application switcher
    /// action to begin.
    ///
    /// Defaults to #mir_input_event_modifier_alt.
    ///
    /// \param modifier the new modifier
    /// \returns this
    ApplicationSwitcherKeyboardFilter& primary_modifier(MirInputEventModifier modifier);

    /// Sets the reverse modifier.
    ///
    /// This is the modifier that must be held alongside the primary modifier
    /// in order to reverse the direction of an action being run. For example,
    /// holding the primary modifier and hitting the application key will select
    /// the next application, while holding both the primary and reverse modifiers
    /// and hitting the application will select the previous application..
    ///
    /// Defaults to #mir_input_event_modifier_shift.
    ///
    /// \param modifier the new modifier
    /// \returns this
    ApplicationSwitcherKeyboardFilter& reverse_modifier(MirInputEventModifier modifier);

    /// Sets the application scancode.
    ///
    /// This is the key that must be clicked in order to trigger either
    /// #miral::ApplicationSwitcher::next_app or #miral::ApplicationSwitcher::prev_app,
    /// depending on the modifiers that are currently being held.
    ///
    /// Defaults to KEY_TAB.
    ///
    /// \param key the key to trigger an application switch
    /// \returns this
    ApplicationSwitcherKeyboardFilter& application_key(int scancode);

    /// Sets the window scancode.
    ///
    /// This is the key that must be clicked in order to trigger either
    /// #miral::ApplicationSwitcher::next_window or #miral::ApplicationSwitcher::prev_window,
    /// depending on the modifiers that are currently being held.
    ///
    /// Defaults to KEY_GRAVE.
    ///
    /// \param key the key to trigger a window switch
    /// \returns this
    ApplicationSwitcherKeyboardFilter& window_key(int scancode);

    operator std::function<bool(MirKeyboardEvent const* event)>() const;

private:
    class Self;
    std::unique_ptr<Self> self;
};

}

#endif
