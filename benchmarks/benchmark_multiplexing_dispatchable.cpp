#include "mir/dispatch/multiplexing_dispatchable.h"
#include "mir/dispatch/simple_dispatch_thread.h"

#include <iostream>
#include <atomic>
#include <vector>
#include <memory>
#include <chrono>
#include <poll.h>
#include <unistd.h>

namespace md = mir::dispatch;

class TestDispatchable : public md::Dispatchable
{
public:
    TestDispatchable(uint64_t limit)
        : dispatch_count{0},
          dispatch_limit{limit}
    {
        int pipefds[2];
        pipe(pipefds);

        read_fd = mir::Fd{pipefds[0]};
        write_fd = mir::Fd{pipefds[1]};

        char dummy{0};
        ::write(write_fd, &dummy, sizeof(dummy));
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
    uint64_t dispatch_count;
    uint64_t const dispatch_limit;
    mir::Fd read_fd, write_fd;
};

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
    dispatcher->add_watch(std::make_shared<TestDispatchable>(dispatch_count), md::DispatchReentrancy::reentrant);

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
