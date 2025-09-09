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

#ifndef MIRAL_APPLICATION_H
#define MIRAL_APPLICATION_H

#include <mir_toolkit/common.h>

#include <memory>
#include <string>
#include <unistd.h>

namespace mir
{
namespace scene { class Session; }
}

namespace miral
{
/// An application that one or more #miral::Window instances are associated with.
///
/// Compositor authors are not expected to initialize this structure themselves.
/// Instead, they may use #miral::WindowManagementPolicy::advise_new_app to be
/// notified when new applications become available.
///
/// \sa miral::WindowManagementPolicy::advise_new_app - notification when a new
///     application has been created
/// \sa miral::WindowManagementPolicy::advise_delete_app - notification when an
///     application has been disconnected
/// \sa miral::WindowManagerTools::info_for - provides access to additional information
///     about the application
using Application = std::shared_ptr<mir::scene::Session>;

[[deprecated("Not meaningful: legacy of mirclient API")]]
void apply_lifecycle_state_to(Application const& application, MirLifecycleState state);

/// Kills the \p application with the signal provided by \p sig.
///
/// \param application The application to kill
/// \param sig The signal to give to the application
void kill(Application const& application, int sig);

/// Retrieve the name of the \p application.
///
/// \param application The application
/// \returns The name of the application, possibly an empty string
///          if it lacks a name.
auto name_of(Application const& application) -> std::string;

/// Retrive the pid of the \p application.
///
/// \param application The application
/// \returns The pid of the application
auto pid_of(Application const& application) -> pid_t;

/// Returns the file descriptor of the client's socket connection, or -1 if
/// there is no client socket.
///
/// May be used for  authentication with apparmor. X11 apps always return -1,
/// since they do not connect directly to the Mir process.
///
/// \param application The application
/// \returns The file descriptor of the client's socket connection, or -1 if there
///          is no client socket.
///
/// \remark Since MirAL 3.4
auto socket_fd_of(Application const& application) -> int;
}

#endif //MIRAL_APPLICATION_H
