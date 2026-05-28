/*
 * Copyright © Canonical Ltd.
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
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

static int resolve_own_path(char const* argv0, char* resolved, size_t resolved_size)
{
    if (resolved_size == 0)
        return -1;

    ssize_t const read_count = readlink("/proc/self/exe", resolved, resolved_size - 1);

    if (read_count > 0 && (size_t)read_count < resolved_size)
    {
        resolved[read_count] = '\0';
        return 0;
    }

    if (argv0 == NULL || *argv0 == '\0')
        return -1;

    if (strchr(argv0, '/') != NULL)
    {
        snprintf(resolved, resolved_size, "%s", argv0);
        return 0;
    }

    return -1;
}

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
    char invocation_path[1024] = "";
    char path[1024] = "", *dest = path, *dest_max = path+sizeof(path)-1;
    char *pivot = path;
    size_t pivot_max = 0;
    const char *src = argv[0];
    char const* const argv0_slash = argv[0] != NULL ? strrchr(argv[0], '/') : NULL;
    char const* const argv0_name = argv0_slash != NULL ? argv0_slash + 1 : argv[0];
    char const* const name = argv0_name;

    (void)argc;

    if (resolve_own_path(argv[0], invocation_path, sizeof(invocation_path)) == 0)
    {
        src = invocation_path;
    }

    while (*src && dest < dest_max)
    {
        *dest = *src;
        if (*dest == '/')
        {
            pivot = dest + 1;
        }
        ++src;
        ++dest;
    }
    pivot_max = dest_max - pivot;
    *dest_max = '\0';

    strncpy(pivot, "../lib/", pivot_max);
    appendenv("LD_LIBRARY_PATH", path);
    printf("LD_LIBRARY_PATH=%s\n", getenv("LD_LIBRARY_PATH"));

    strncpy(pivot+7, "server-modules/", pivot_max-7);
    setenv("MIR_SERVER_PLATFORM_PATH", path, 1);
    printf("MIR_SERVER_PLATFORM_PATH=%s\n", path);

    snprintf(pivot, pivot_max, EXECUTABLE_FORMAT, name);
    *dest_max = '\0';
    printf("exec=%s\n", path);

    argv[0] = path;
    execv(argv[0], argv);

    fprintf(stderr, "Failed to execute: %s\n", path);
    return 1;
}
