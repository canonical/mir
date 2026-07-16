/*
 * Copyright © Canonical Ltd.
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
 */
#ifndef MIR_PROOF_OF_MUTEX_LOCK_H_
#define MIR_PROOF_OF_MUTEX_LOCK_H_

#include <mir/fatal.h>
#include <mutex>

namespace mir
{
/// A method can take an instance of this class by reference to require callers to hold a mutex lock, without requiring
/// a specific type of lock
class ProofOfMutexLock
{
public:
    ProofOfMutexLock(std::lock_guard<std::mutex> const&) {}
    ProofOfMutexLock(std::unique_lock<std::mutex> const& lock)
    {
        if (!lock.owns_lock())
        {
            MIR_FATAL_ERROR("ProofOfMutexLock created with unlocked unique_lock");
        }
    }
    ProofOfMutexLock(ProofOfMutexLock const&) = delete;
    ProofOfMutexLock operator=(ProofOfMutexLock const&) = delete;
};
} // namespace mir

#endif // MIR_PROOF_OF_MUTEX_LOCK_H_
