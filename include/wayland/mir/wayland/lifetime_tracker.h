/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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

#ifndef MIR_WAYLAND_LIFETIME_TRACKER_H_
#define MIR_WAYLAND_LIFETIME_TRACKER_H_

#include <mir/int_wrapper.h>

#include <memory>
#include <functional>

namespace mir
{
namespace wayland
{
namespace detail
{
struct DestroyListenerIdTag;
}

typedef IntWrapper<detail::DestroyListenerIdTag> DestroyListenerId;

/// The base class of any object that wants to provide a destroyed flag
/// The destroyed flag is only created when needed and automatically set to true on destruction
/// This pattern is only safe in a single-threaded context
class LifetimeTracker
{
public:
    LifetimeTracker();
    LifetimeTracker(LifetimeTracker const&) = delete;
    LifetimeTracker& operator=(LifetimeTracker const&) = delete;

    virtual ~LifetimeTracker();
    /// The pointed-at bool contains false if this object is still alive and true if it has been destroyed.
    auto destroyed_flag() const -> std::shared_ptr<bool const>;
    /// The given function will be called just before the object is marked as destroyed. The returned ID can be used
    /// to remove the listener in which case it is never called. DestroyListenerId{} (value 0) is never returned, and so
    /// it can be used as a null ID. Destroy listener call order is undefined.
    auto add_destroy_listener(std::function<void()> listener) const -> DestroyListenerId;
    /// If the given ID maps to a destroy listener, that listener is dropped without being called. If the listener has
    /// already been dropped or never existed, this call is ignored.
    void remove_destroy_listener(DestroyListenerId id) const;

protected:
    /// Subclasses are not required to call this, but may do so during the destruction process if the object needs to
    /// get marked as destroyed and fire its destroy listeners before some other part of the destructor runs.
    void mark_destroyed() const;

private:
    struct Impl;

    /// Since many Wayland objects are created and the features of this class are used for only a few, impl is created
    /// lazily to conserve memory.
    std::unique_ptr<Impl> mutable impl;
};
}
}

#endif // MIR_WAYLAND_LIFETIME_TRACKER_H_
