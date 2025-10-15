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

namespace miral
{
class WaylandExtensions;

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
/// This internal client automatically enables and uses the following protocols:
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
    ///
    /// \param extensions A instance of #miral::WaylandExtensions
    explicit ApplicationSwitcher(WaylandExtensions& extensions);
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
}

#endif
