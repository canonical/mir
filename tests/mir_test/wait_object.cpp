/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 *              Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "mir/test/wait_object.h"

namespace mt = mir::test;

void mt::WaitObject::notify_ready()
{
    std::unique_lock<std::mutex> lock{mutex};
    ready = true;
    cv.notify_all();
}

void mt::WaitObject::wait_until_ready()
{
    wait_until_ready(std::chrono::seconds{10});
}

