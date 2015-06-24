/*
 * Copyright © 2013 Canonical Ltd.
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
 */

#include "mir/test/cross_process_action.h"

namespace mt = mir::test;

void mt::CrossProcessAction::exec(std::function<void()> const& f)
{
    start_sync.wait_for_signal_ready();
    f();
    finish_sync.signal_ready();
}

void mt::CrossProcessAction::operator()(std::chrono::milliseconds timeout)
{
    start_sync.signal_ready();
    finish_sync.wait_for_signal_ready_for(timeout);
}
