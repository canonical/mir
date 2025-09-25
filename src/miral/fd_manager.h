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

#ifndef MIRAL_FD_MANAGER_H
#define MIRAL_FD_MANAGER_H

#include "miral/runner.h"
#include "mir/fd.h"

#include <memory>
#include <mutex>
#include <vector>
#include <functional>

namespace mir { class MainLoop; }

namespace miral
{
/// Used to create FdHandles which registers file descriptors onto the Server
/// and automatically deregisters them once the handle is dropped
class FdManager : public std::enable_shared_from_this<FdManager>
{
public:
    FdManager();
    ~FdManager();

    auto register_handler(mir::Fd fd, std::function<void(int)> const& handler)
    -> std::unique_ptr<FdHandle>;

    void unregister_handler(void const* owner);

    void set_main_loop(std::shared_ptr<mir::MainLoop> main_loop);

private:
    /// A struct holding the necessary info to register a file descriptor if FdManager::register_handler()
    /// is called before the Server has started.
    struct FdInfo
    {
        mir::Fd fd;
        void const* owner;
        std::function<void(int)> handler;
    };

    std::mutex mutex;
    std::weak_ptr<mir::MainLoop> weak_main_loop;

    // Backlog of FdInfo used to register all handlers
    // sent to register_handler() before the Server started
    std::vector<FdInfo> backlog;
};

struct FdHandleImpl : public FdHandle
{
public:
    FdHandleImpl(mir::Fd fd, std::shared_ptr<FdManager> manager);
    ~FdHandleImpl() override;
    FdHandleImpl(FdHandleImpl&&) = default;

private:
    friend class FdManager;

    FdHandleImpl(FdHandle const&) = delete;
    FdHandleImpl& operator=(FdHandleImpl const&) = delete;

    mir::Fd fd;
    std::shared_ptr<FdManager> manager;
};
}

#endif //MIRAL_FD_MANAGER_H
