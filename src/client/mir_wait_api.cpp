/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
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

#include "mir_toolkit/mir_wait.h"
#include "mir_wait_handle.h"

void mir_wait_for(MirWaitHandle* wait_handle)
{
    if (wait_handle)
        wait_handle->wait_for_all();
}

void mir_wait_for_one(MirWaitHandle* wait_handle)
{
    if (wait_handle)
        wait_handle->wait_for_one();
}
