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

#ifndef MIR_FRONTEND_EXECUTOR_H
#define MIR_FRONTEND_EXECUTOR_H

#include <mir/executor.h>

#include <rust/cxx.h>

#include <mutex>
#include <deque>
#include <functional>
#include <optional>

namespace mir
{
namespace wayland_rs
{
struct WaylandEventLoopHandle;
}
namespace frontend
{
class WaylandExecutor : public Executor
{
public:
    WaylandExecutor();
    ~WaylandExecutor();

    void spawn(std::function<void()>&& work) override;

    /// Connect the executor to the Rust event loop.
    /// Called from the event loop thread once the loop is ready.
    /// Any work buffered before this call will be flushed.
    void set_loop_handle(rust::Box<wayland_rs::WaylandEventLoopHandle> handle);

private:
    std::mutex mutex;
    std::optional<rust::Box<wayland_rs::WaylandEventLoopHandle>> loop_handle;
    std::deque<std::function<void()>> pending;
};
}
}

#endif //MIR_FRONTEND_EXECUTOR_H
