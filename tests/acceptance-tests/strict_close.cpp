/*
 * Copyright © 2016 Canonical Ltd.
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


#include <dlfcn.h>
#include <unistd.h>
#include <iostream>
#include <cstdlib>

int close(int fd)
{
    static int (*real_close)(int) = nullptr;

    if (!real_close)
    {
        real_close = reinterpret_cast<int(*)(int)>(dlsym(RTLD_NEXT, "close"));
    }

    auto result = real_close(fd);

    if (result == -1 && errno == EBADF)
    {
        std::cerr
            << "Detected attempt to close a bad file-descriptor.\n"
            << "This usually indicates a double-close bug.\n"
            << "The bad file descriptor was: " << fd << std::endl;
        abort();
    }
    return result;
}
