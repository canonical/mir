/*
 * Copyright © 2014-2015 Canonical Ltd.
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
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 *              Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

#ifdef __cplusplus
#error "This test only works properly as a C program not linked to C++."
#endif

int main(int argc, char** argv)
{
    int i;
    char const* libname = argc <= 1 ? DEFAULT_LIB_NAME : argv[1];
    for (i = 0; i < 10; i++)
    {
        void* dl = NULL;
        printf("[%d] dlopen `%s' = ", i, libname);
        dl = dlopen(libname, RTLD_NOW);
        if (dl)
        {
            printf("%p\n", dl);
            dlclose(dl);
        }
        else
        {
            printf("NULL (%s)\n", dlerror());
            return 1;   /* Non-zero means polite test failure */
        }
    }
    return 0;
}
