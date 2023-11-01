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

namespace
{
int shutdown_signal;
void trigger_shutdown(int signum)
{
    if (eventfd_write(shutdown_signal, 1) == -1)
    {
        printf("Failed to execute a shutdown");
        exit(-1);
    }
    else
    {
        printf("Signal %d received. Good night.\n", signum);
    }
}
}

void mir::client::WaylandRunner::operator()(wl_display* display)
{
    enum FdIndices {
        display_fd = 0,
        shutdown,
        indices
    };

    struct pollfd fds[indices] = {
        {wl_display_get_fd(display), POLLIN, 0},
        {shutdown_signal,            POLLIN, 0},
    };

    wl_display_roundtrip(display);
    wl_display_roundtrip(display);
    while (!(fds[shutdown].revents & (POLLIN | POLLERR)))
    {
        while (wl_display_prepare_read(display) != 0)
        {
            switch (wl_display_dispatch_pending(display))
            {
            case -1:
                fprintf(stderr, "Failed to dispatch Wayland events\n");
                break;
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
            }
        }
        else
        {
            wl_display_cancel_read(display);
        }
    }
}

mir::client::WaylandRunner::WaylandRunner()
{
    if (shutdown_signal)
        abort();

    shutdown_signal = eventfd(0, EFD_CLOEXEC);

    struct sigaction sig_handler_new;
    sigfillset(&sig_handler_new.sa_mask);
    sig_handler_new.sa_flags = 0;
    sig_handler_new.sa_handler = trigger_shutdown;

    sigaction(SIGINT, &sig_handler_new, NULL);
    sigaction(SIGTERM, &sig_handler_new, NULL);
    sigaction(SIGHUP, &sig_handler_new, NULL);
}

void mir::client::WaylandRunner::run(wl_display* display)
{
    static WaylandRunner the_instance;
    the_instance(display);
}
