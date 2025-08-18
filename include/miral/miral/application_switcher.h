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

/// A headless application switcher.
///
/// This class spawns an internal wayland client. Shells can bind the methods
/// defined in this class to custom keybinds. For example, a shell mayb bind "alt + tab"
/// to trigger [ApplicationSwitcher::next_app].
///
/// Instances of this class should be provided to [miral::InternalClientLauncher::launch].
///
/// This class relies on the wlr foreign toplevel management being available in your
/// shell.
class ApplicationSwitcher
{
public:
    /// Creates a new application switcher.
    ApplicationSwitcher();

    void operator()(struct wl_display* display);
    void operator()(std::weak_ptr<mir::scene::Session> const& session) const;

    void next_app();
    void prev_app();

    void stop() const;

private:
    struct Self;
    std::shared_ptr<Self> self;
};
}

#endif