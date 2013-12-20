/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir_test_framework/detect_server.h"

#include <chrono>
#include <thread>

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

bool mir_test_framework::detect_server(
    std::string const& socket_file,
    std::chrono::milliseconds const& timeout)
{
    auto limit = std::chrono::steady_clock::now() + timeout;

    bool error = false;
    struct stat file_status;

    do
    {
        if (error)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(0));
        }
        error = stat(socket_file.c_str(), &file_status);
    }
    while (error && std::chrono::steady_clock::now() < limit);

    return !error;
}

