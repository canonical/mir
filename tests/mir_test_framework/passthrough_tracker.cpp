/*
 * Copyright © 2019 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir_test_framework/passthrough_tracker.h"

namespace mtf = mir_test_framework;

void mtf::PassthroughTracker::note_passthrough()
{
    std::unique_lock<std::mutex> lk(mutex);
    num_passthrough++;
    cv.notify_all();
}

bool mtf::PassthroughTracker::wait_for_passthrough_frames(size_t nframes, std::chrono::milliseconds ms)
{
    std::unique_lock<std::mutex> lk(mutex);
    return cv.wait_for(lk, ms, [&] { return num_passthrough >= nframes; } );
}

