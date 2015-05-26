/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "mir/dispatch/multiplexing_dispatchable.h"

#include <iostream>
#include <vector>
#include <memory>
#include <chrono>
#include <thread>
#include <poll.h>
#include <unistd.h>

namespace md = mir::dispatch;

class TestDispatchable : public md::Dispatchable
{
public:
    TestDispatchable(uint64_t limit)
        : dispatch_limit{limit}
    {
        int pipefds[2];
        if (pipe(pipefds) < 0)
        {
            throw std::system_error{errno, std::system_category(), "Failed to create pipe"};
        }

        read_fd = mir::Fd{pipefds[0]};
        write_fd = mir::Fd{pipefds[1]};

        char dummy{0};
        if (::write(write_fd, &dummy, sizeof(dummy)) != sizeof(dummy))
        {
            throw std::system_error{errno, std::system_category(), "Failed to mark dispatchable"};
        }
    }

    mir::Fd watch_fd() const override
    {
        return read_fd;
    }
    bool dispatch(md::FdEvents) override
    {
        ++dispatch_count;
        return (dispatch_count < dispatch_limit);
    }
    md::FdEvents relevant_events() const override
    {
        return md::FdEvent::readable;
    }

private:
    static thread_local uint64_t dispatch_count;
    uint64_t const dispatch_limit;
    mir::Fd read_fd, write_fd;
};

thread_local uint64_t TestDispatchable::dispatch_count = 0;

bool fd_is_readable(int fd)
{
    struct pollfd poller {
        fd,
        POLLIN,
        0
    };
    return poll(&poller, 1, 0);
}

int main(int argc, char** argv)
{
    if (argc != 3)
    {
        std::cout<<"Usage: "<<argv[0]<<" <number of threads> <dispatch count>"<<std::endl;
        exit(1);
    }

    int const thread_count = std::atoi(argv[1]);
    uint64_t const dispatch_count = std::atoll(argv[2]);

    auto dispatcher = std::make_shared<md::MultiplexingDispatchable>();
    dispatcher->add_watch(std::make_shared<TestDispatchable>(dispatch_count / thread_count), md::DispatchReentrancy::reentrant);

    auto start = std::chrono::steady_clock::now();

    std::vector<std::thread> thread_loops;
    for (int i = 0; i < thread_count; ++i)
    {
        thread_loops.emplace_back([](md::Dispatchable& dispatch)
        {
            while(fd_is_readable(dispatch.watch_fd()))
            {
                dispatch.dispatch(md::FdEvent::readable);
            }
        }, std::ref(*dispatcher));
    }

    for (auto& thread : thread_loops)
    {
        thread.join();
    }

    auto duration = std::chrono::steady_clock::now() - start;
    std::cout<<"Dispatching "<<dispatch_count<<" times took "<<std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count()<<"ns"<<std::endl;
    exit(0);
}
