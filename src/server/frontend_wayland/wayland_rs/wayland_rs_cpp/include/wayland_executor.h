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

#ifndef MIR_WAYLANDRS_WAYLAND_EXECUTOR
#define MIR_WAYLANDRS_WAYLAND_EXECUTOR

#include <mir/executor.h>

#include <functional>
#include <mutex>
#include <vector>

namespace mir
{
namespace wayland_rs
{
struct WaylandServer;

/// An Executor that runs work on the Wayland server's event loop.
///
/// Because C++ functions cannot be ferried across the C++/Rust boundary, work
/// is held in a queue on the C++ side. `spawn` appends work and, when no signal
/// is already outstanding, asks the running event loop to drain it via
/// `WaylandServer::drain_queue`. Rust later calls back into `execute` on the
/// event-loop thread, which drains and runs all pending work.
///
/// `spawn` may be called from any thread. Queued work is run in the order it
/// was spawned. The server must be running for queued work to be executed; a
/// signal raised before the server is running is dropped, so `spawn` re-signals
/// until one is delivered.
class WaylandExecutor : public mir::Executor
{
public:
    explicit WaylandExecutor(WaylandServer& server);

    /// Schedule work to be run on the Wayland event loop thread.
    void spawn(std::function<void()>&& work) override;

    /// Run all currently queued work. Called from Rust on the event-loop
    /// thread.
    void execute();

private:
    WaylandServer& server;
    std::mutex mutex;
    bool signal_pending{false};
    std::vector<std::function<void()>> pending_work;
};

}
}

#endif  // MIR_WAYLANDRS_WAYLAND_EXECUTOR
