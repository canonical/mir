/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

static void appendenv(const char* varname, const char* append)
{
    char buf[2048] = "";
    const char* value = append;
    const char* old = getenv(varname);
    if (old != NULL)
    {
        snprintf(buf, sizeof(buf)-1, "%s:%s", old, append);
        buf[sizeof(buf)-1] = '\0';
        value = buf;
    }
    setenv(varname, value, 1);
}

int main(int argc, char** argv)
{
    char path[1024], *dest = path, *dest_max = path+sizeof(path)-1;
    char *pivot = path;
    size_t pivot_max = 0;
    const char *src = argv[0], *name = argv[0];

    (void)argc;
    while (*src && dest < dest_max)
    {
        *dest = *src;
        if (*dest == '/')
        {
            pivot = dest + 1;
            name = src + 1;
        }
        ++src;
        ++dest;
    }
    pivot_max = dest_max - pivot;

    strncpy(pivot, "../lib/client-modules/", pivot_max);
    *dest_max = '\0';
    setenv("MIR_CLIENT_PLATFORM_PATH", path, 1);
    printf("MIR_CLIENT_PLATFORM_PATH=%s\n", path);
    strncpy(pivot+7, "server-modules/", pivot_max-7);
    setenv("MIR_SERVER_PLATFORM_PATH", path, 1);
    printf("MIR_SERVER_PLATFORM_PATH=%s\n", path);

    pivot[6] = '\0';  /* truncate lib/client-modules to just lib */
    appendenv("LD_LIBRARY_PATH", path);
    printf("LD_LIBRARY_PATH=%s\n", getenv("LD_LIBRARY_PATH"));

    snprintf(pivot, pivot_max, EXECUTABLE_FORMAT, name);
    *dest_max = '\0';
    printf("exec=%s\n", path);

    argv[0] = path;
    execv(argv[0], argv);

    fprintf(stderr, "Failed to execute: %s\n", path);
    return 1;
}
