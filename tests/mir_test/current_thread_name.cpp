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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/test/current_thread_name.h"

#include <pthread.h>

std::string mir::test::current_thread_name()
{
    static size_t const max_thread_name_size = 16;
    char thread_name[max_thread_name_size] = "unknown";

#ifndef MIR_DONT_USE_PTHREAD_GETNAME_NP
    pthread_getname_np(pthread_self(), thread_name, sizeof thread_name);
#endif

    return {thread_name};
}
