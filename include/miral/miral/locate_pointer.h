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

#ifndef MIRAL_LOCATE_POINTER_H
#define MIRAL_LOCATE_POINTER_H

#include <mir/geometry/forward.h>
#include <miral/toolkit_event.h>

#include <chrono>
#include <functional>
#include <memory>

namespace mir
{
class Server;
}

namespace miral
{
namespace live_config { class Store; }
class LocatePointer
{
public:
    /// Creates a `LocatePointer` instance that's enabled by default.
    static auto enabled() -> LocatePointer;

    /// Creates a `LocatePointer` instance that's disabled by default.
    static auto disabled() -> LocatePointer;

    void operator()(mir::Server& server);

    /// Construct a `LocatePointer` instance with access to a live config
    /// store.
    ///
    /// Available options:
    ///     - {locate_pointer, enable}: Enable or disable locate pointer.
    ///     - {locate_pointer, delay}: The delay between scheduling a request
    ///     and the #on_locate_pointer callback being called.
    explicit LocatePointer(miral::live_config::Store& config_store);

    /// The delay between scheduling a request and the #on_locate_pointer
    /// callback being called.
    /// \note The default delay is 500 milliseconds
    LocatePointer& delay(std::chrono::milliseconds delay);

    /// Configures the callback that's invoked after #schedule_request is
    /// called by #delay milliseconds.
    /// Useful for providing feedback to users.
    LocatePointer& on_locate_pointer(std::function<void(mir::geometry::PointF pointer_position)>&&);

    // Enables locate pointer.
    // When already enabled, further calls have no effect.
    LocatePointer& enable();

    // Disables locate pointer.
    // When already disabled, further calls have no effect.
    LocatePointer& disable();

    /// Schedules the callback provided to #on_locate_pointer to be called
    /// with the current mouse position at a point in the future specified by
    /// #delay. If #cancel_request is called after this, then the callback
    /// will not be called.
    ///
    /// \note If the #LocatePointer instance is disabled, this method has no
    /// effect.
    void schedule_request();

    /// Cancels the request established by #schedule_request. If a request
    /// is not pending, then nothing happens.
    ///
    /// \note If the #LocatePointer instance is disabled, this method has no
    /// effect.
    void cancel_request();

private:
    struct Self;
    explicit LocatePointer(std::shared_ptr<Self>);
    std::shared_ptr<Self> self;
};
}

#endif // MIRAL_LOCATE_POINTER_H
