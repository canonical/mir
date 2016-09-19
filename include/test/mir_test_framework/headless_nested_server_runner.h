/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored By: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_TEST_FRAMEWORK_HEADLESS_NESTED_SERVER_RUNNER_H_
#define MIR_TEST_FRAMEWORK_HEADLESS_NESTED_SERVER_RUNNER_H_

#include "mir_test_framework/async_server_runner.h"
#include <atomic>

namespace mir_test_framework
{
struct PassthroughTracker
{
    size_t num_passthrough_frames();
    void note_passthrough();
private:
    std::atomic<size_t> num_optimized{0u};
};

class HeadlessNestedServerRunner : public AsyncServerRunner
{
public:
    HeadlessNestedServerRunner(std::string const& connect_string);
    size_t num_passthrough_frames() const;
private:
    std::shared_ptr<PassthroughTracker> const passthrough_tracker;
};
}

#endif /* MIR_TEST_FRAMEWORK_HEADLESS_NESTED_SERVER_RUNNER_H_ */
