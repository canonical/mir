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

#include "work_callback.h"

#include <mir/executor.h>

#include <cstdint>
#include <functional>
#include <mutex>
#include <unordered_map>

namespace mir
{
namespace wayland_rs
{
struct WaylandServer;

/// An Executor that runs work on the Wayland server's event loop.
///
/// Because C++ functions cannot be ferried across the C++/Rust boundary, each
/// unit of work is stored on the C++ side and identified by an integer id. The
/// id is handed to the running event loop via `WaylandServer::schedule_work`;
/// Rust later calls back into `execute` on the event-loop thread to run it.
///
/// `spawn` may be called from any thread. Scheduled work is run in the order it
/// was spawned. The server must be running for spawned work to be executed.
class WaylandExecutor : public mir::Executor, public WorkCallback
{
public:
    explicit WaylandExecutor(WaylandServer& server);

    /// Schedule work to be run on the Wayland event loop thread.
    void spawn(std::function<void()>&& work) override;

    /// Run a previously scheduled unit of work. Called from Rust on the
    /// event-loop thread.
    auto execute(std::uint32_t work_id) -> void override;

private:
    WaylandServer& server;
    std::mutex mutex;
    std::uint32_t next_id{0};
    std::unordered_map<std::uint32_t, std::function<void()>> pending_work;
};

}
}

#endif  // MIR_WAYLANDRS_WAYLAND_EXECUTOR
