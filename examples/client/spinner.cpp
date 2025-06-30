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

#include "splash.h"
#include <wayland-client.h>
#include <csignal>

static void shutdown(int)
{
    // NB we can't safely call exit() from a signal handler
    _Exit(EXIT_SUCCESS);
}

int main()
{
    struct sigaction action{};
    action.sa_handler = shutdown;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTERM, &action, NULL);

    SpinnerSplash spinner;

    auto const display = wl_display_connect(nullptr);

    spinner(display);

    wl_display_disconnect(display);

    return EXIT_SUCCESS;
}
