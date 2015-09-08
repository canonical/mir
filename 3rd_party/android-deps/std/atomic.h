/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_ANDROID_UBUNTU_ATOMIC_H_
#define MIR_ANDROID_UBUNTU_ATOMIC_H_

#include <atomic>
#include <stdint.h>

namespace mir_input
{

/*
 * A handful of basic atomic operations.  The appropriate <atomic>
 * functions should be used instead of these whenever possible.
 * Only those functions needed by the input code are actually supported.
 * Others from the corresponding android cutils header are commented out.
  */

typedef std::atomic<int32_t> android_atomic_int32_t;

/*
 * Basic arithmetic and bitwise operations.  These all provide a
 * barrier with "release" ordering, and return the previous value.
 *
 * These have the same characteristics (e.g. what happens on overflow)
 * as the equivalent non-atomic C operations.
 */
inline int32_t android_atomic_inc(android_atomic_int32_t* addr) { return addr->fetch_add(1); }
inline int32_t android_atomic_dec(android_atomic_int32_t* addr) { return addr->fetch_add(-1); }
inline int32_t android_atomic_add(int32_t value, android_atomic_int32_t* addr) { return addr->fetch_add(value); }
inline int32_t android_atomic_or(int32_t value, android_atomic_int32_t* addr) { return addr->fetch_or(value); }

/*
 * Compare-and-set operation with "acquire" or "release" ordering.
 *
 * This returns zero if the new value was successfully stored, which will
 * only happen when *addr == oldvalue.
 *
 * (The return value is inverted from implementations on other platforms,
 * but matches the ARM ldrex/strex result.)
 *
 * Implementations that use the release CAS in a loop may be less efficient
 * than possible, because we re-issue the memory barrier on each iteration.
 */
inline int android_atomic_release_cas(int32_t oldvalue, int32_t newvalue,
    android_atomic_int32_t* addr) { return !addr->compare_exchange_strong(oldvalue, newvalue); }
}

/*
 * Aliases for code using an older version of this header.  These are now
 * deprecated and should not be used.  The definitions will be removed
 * in a future release.
 */
#define android_atomic_cmpxchg android_atomic_release_cas

using mir_input::android_atomic_int32_t;
using mir_input::android_atomic_inc;
using mir_input::android_atomic_dec;
using mir_input::android_atomic_add;
using mir_input::android_atomic_or;
using mir_input::android_atomic_release_cas;

#endif /* MIR_ANDROID_UBUNTU_ATOMIC_H_ */
