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

#include "wayland_executor.h"
#include "wayland_rs/wayland_rs_cpp/include/work_callback.h"
#include "wayland_rs/src/ffi.rs.h"

#include <mir/log.h>

namespace mf = mir::frontend;
namespace mw = mir::wayland_rs;

namespace
{
class FunctionWorkCallback : public mw::WorkCallback
{
public:
    explicit FunctionWorkCallback(std::function<void()>&& work)
        : work{std::move(work)}
    {
    }

    auto execute() -> void override
    {
        try
        {
            work();
        }
        catch (...)
        {
            mir::log(
                mir::logging::Severity::critical,
                MIR_LOG_COMPONENT,
                std::current_exception(),
                "Exception processing Wayland event loop work item");
        }
    }

private:
    std::function<void()> work;
};
}

mf::WaylandExecutor::WaylandExecutor() = default;
mf::WaylandExecutor::~WaylandExecutor() = default;

void mf::WaylandExecutor::spawn(std::function<void()>&& work)
{
    std::lock_guard lock{mutex};
    if (loop_handle)
    {
        (*loop_handle)->spawn(std::make_unique<FunctionWorkCallback>(std::move(work)));
    }
    else
    {
        pending.emplace_back(std::move(work));
    }
}

void mf::WaylandExecutor::set_loop_handle(rust::Box<mw::WaylandEventLoopHandle> handle)
{
    std::lock_guard lock{mutex};
    loop_handle = std::move(handle);

    for (auto& work : pending)
    {
        (*loop_handle)->spawn(std::make_unique<FunctionWorkCallback>(std::move(work)));
    }
    pending.clear();
}
