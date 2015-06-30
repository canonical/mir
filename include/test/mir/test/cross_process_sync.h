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
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#ifndef MIR_TEST_CROSS_PROCESS_SYNC_H_
#define MIR_TEST_CROSS_PROCESS_SYNC_H_

#include <chrono>

namespace mir
{
namespace test
{
// A cross-process synchronization primitive that supports simple
// wait-condition-like scenarios.
class CrossProcessSync
{
  public:
    CrossProcessSync();
    CrossProcessSync(const CrossProcessSync& rhs);
    ~CrossProcessSync() noexcept;

    CrossProcessSync& operator=(const CrossProcessSync& rhs);

    // Try to signal the other side that we are ready for at most duration milliseconds.
    // Throws a std::runtime_error if not successful.
    void try_signal_ready_for(const std::chrono::milliseconds& duration);

    void try_signal_ready_for();

    // Wait for the other sides to signal readiness for at most duration milliseconds.
    // Returns the number of ready signals that have been collected since creation.
    // Throws std::runtime_error if not successful.
    unsigned int wait_for_signal_ready_for(const std::chrono::milliseconds& duration);
    unsigned int wait_for_signal_ready_for();

    void signal_ready();
    unsigned int wait_for_signal_ready();

  private:
    int fds[2];
    unsigned int counter;
};
}
}

#endif // MIR_TEST_CROSS_PROCESS_SYNC_H_
