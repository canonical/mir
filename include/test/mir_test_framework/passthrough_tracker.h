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

#ifndef MIR_TEST_FRAMEWORK_PASSTHROUGH_TRACKER_H
#define MIR_TEST_FRAMEWORK_PASSTHROUGH_TRACKER_H

#include <chrono>
#include <condition_variable>
#include <mutex>

namespace mir_test_framework
{
struct PassthroughTracker
{
    bool wait_for_passthrough_frames(size_t num_frames, std::chrono::milliseconds ms);
    void note_passthrough();

private:
    std::mutex mutex;
    std::condition_variable cv;
    size_t num_passthrough;
};
}

#endif //MIR_TEST_FRAMEWORK_PASSTHROUGH_TRACKER_H
