/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIRAL_APPLICATION_INFO_H
#define MIRAL_APPLICATION_INFO_H

#include "miral/application.h"

#include <string>
#include <vector>

namespace miral
{
class Window;

/// Provides information about a #miral::Application and its associated
/// list of #miral::Window objects.
///
/// An instance of this class may be obtained from
/// #miral::WindowManagerTools::info_for(Application).
///
/// \sa miral::Application - the class for which this class provides information
/// \sa miral::WindowManagerTools::info_for(Application) - the function to obtain an instance of this class
struct ApplicationInfo
{
    /// Constructs a new application info without a backing #miral::Application.
    ApplicationInfo();

    /// Constructs a new application info backed by \p app.
    ///
    /// \param app The backing application.
    explicit ApplicationInfo(Application const& app);
    ~ApplicationInfo();
    ApplicationInfo(ApplicationInfo const& that);
    auto operator=(ApplicationInfo const& that) -> miral::ApplicationInfo&;

    /// Retrieve the name of the application.
    ///
    /// \returns The name of the application.
    auto name() const -> std::string;

    /// Retrieve the backing #miral::Application for this info.
    ///
    /// \returns The backing application.
    auto application()  const -> Application;

    /// Retrieve the list of #miral::Window instances that are associated with this
    /// #miral::Application.
    ///
    /// \returns The list of windows associated with the application.
    auto windows() const -> std::vector <Window>&;

    /// Retrieve the user data set for this application.
    ///
    /// Window manager authors may set this payload with
    /// #miral::ApplicationInfo::userdata(std::shared_ptr<void>).
    ///
    /// \returns The user data set by the compositor author, and nullptr
    ///          if none was set.
    auto userdata() const -> std::shared_ptr<void>;

    /// Set the arbitrary user data payload for this application.
    ///
    /// This can be used by the window manager author to store information
    /// specific to their use case.
    ///
    /// This payload can be retrieved later via #miral::ApplicationInfo::userdata().
    ///
    /// \param userdata An arbitrary payload.
    void userdata(std::shared_ptr<void> userdata);

private:
    friend class BasicWindowManager;
    void add_window(Window const& window);
    void remove_window(Window const& window);

    struct Self;
    std::unique_ptr<Self> const self;
};
}

#endif //MIRAL_APPLICATION_INFO_H
