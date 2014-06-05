/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/thread_name.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <pthread.h>

void mir::set_thread_name(std::string const& name)
{
    static size_t const max_name_len = 15;
    auto const proper_name = name.substr(0, max_name_len);

    pthread_setname_np(pthread_self(), proper_name.c_str());
}
