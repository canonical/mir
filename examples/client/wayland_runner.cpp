/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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

#include "wayland_runner.h"

#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <sys/eventfd.h>

#include <boost/throw_exception.hpp>

#include <list>
#include <mutex>
#include <system_error>

namespace
{

// We use a namespace object constructor to install our signal handler once
struct State
{
    State();

    void run(wl_display* display);
    void quit();

    static int shutdown_fd;
};

State state;
int State::shutdown_fd = 0;

extern "C" void signal_shutdown(int signum)
{
    printf("Signal %d received. Good night.\n", signum);
    state.quit();
}

class ActionQueue
{
public:
    using FdEvents = uint32_t;

    ActionQueue();
    ~ActionQueue();
    int watch_fd() const;

    void enqueue(std::function<void()> const&& action);

    bool dispatch(FdEvents events);

private:
    bool consume();
    void wake();
    int event_fd;
    std::mutex list_lock;
    std::list<std::function<void()>> actions;
};

ActionQueue action_queue;
}

void State::run(wl_display* display)
{
    enum FdIndices {
        display_fd = 0,
        actions,
        shutdown,
        indices
    };

    struct pollfd fds[indices] = {
        {wl_display_get_fd(display), POLLIN, 0},
        {action_queue.watch_fd(),    POLLIN, 0},
        {shutdown_fd,                POLLIN, 0},
    };

    while (!(fds[shutdown].revents & (POLLIN | POLLERR)))
    {
        while (wl_display_prepare_read(display) != 0)
        {
            switch (wl_display_dispatch_pending(display))
            {
            case -1:
                fprintf(stderr, "Failed to dispatch Wayland events\n");
                abort();
            case 0:
                break;
            default:
                wl_display_flush(display);
            }
        }

        if (poll(fds, indices, -1) == -1)
        {
            fprintf(stderr, "Failed to poll\n");
            wl_display_cancel_read(display);
            break;
        }

        if (fds[display_fd].revents & (POLLIN | POLLERR))
        {
            if (wl_display_read_events(display))
            {
                fprintf(stderr, "Failed to read Wayland events\n");
                abort();
            }
        }
        else
        {
            wl_display_cancel_read(display);
        }

        action_queue.dispatch(fds[actions].revents);
    }
}

State::State()
{
    shutdown_fd = eventfd(0, EFD_CLOEXEC);

    struct sigaction sig_handler_new;
    sigfillset(&sig_handler_new.sa_mask);
    sig_handler_new.sa_flags = 0;
    sig_handler_new.sa_handler = signal_shutdown;

    sigaction(SIGINT, &sig_handler_new, NULL);
    sigaction(SIGTERM, &sig_handler_new, NULL);
    sigaction(SIGHUP, &sig_handler_new, NULL);
}

void State::quit()
{
    if (eventfd_write(shutdown_fd, 1) == -1)
    {
        printf("Failed to execute a shutdown");
        exit(-1);
    }
}

void mir::client::wayland_runner::run(wl_display* display)
{
    state.run(display);
}

void mir::client::wayland_runner::quit()
{
    state.quit();
}

void mir::client::wayland_runner::spawn(std::function<void()>&& function)
{
    action_queue.enqueue(std::move(function));
}

ActionQueue::ActionQueue()
    : event_fd{eventfd(0, EFD_CLOEXEC|EFD_NONBLOCK|EFD_SEMAPHORE)}
{
    if (event_fd < 0)
        BOOST_THROW_EXCEPTION((std::system_error{errno,
                                                 std::system_category(),
                                                 "Failed to create event fd for action queue"}));
}

int ActionQueue::watch_fd() const
{
    return event_fd;
}

void ActionQueue::enqueue(std::function<void()> const&& action)
{
    std::unique_lock lock(list_lock);
    actions.push_back(action);
    wake();
}

bool ActionQueue::dispatch(FdEvents events)
{
    if (events & POLLERR)
        return false;

    if (!consume())
    {
        // Another thread has won the race to process this event
        // That's OK, just bail.
        return true;
    }

    std::function<void()> action_to_process;

    {
        std::unique_lock lock(list_lock);
        action_to_process = actions.front();
        actions.pop_front();
    }

    action_to_process();

    return true;
}

bool ActionQueue::consume()
{
    uint64_t num_actions;
    if (read(event_fd, &num_actions, sizeof num_actions) != sizeof num_actions)
    {
        if (errno == EAGAIN)
        {
            return false;
        }
        BOOST_THROW_EXCEPTION((std::system_error{errno,
                                                 std::system_category(),
                                                 "Failed to consume action queue notification"}));
    }
    return true;
}

void ActionQueue::wake()
{
    uint64_t one_more{1};
    if (write(event_fd, &one_more, sizeof one_more) != sizeof one_more)
        BOOST_THROW_EXCEPTION((std::system_error{errno,
                                                 std::system_category(),
                                                 "Failed to wake action queue"}));
}

ActionQueue::~ActionQueue()
{
    close(event_fd);
}
