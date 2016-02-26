/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#ifndef MIR_TEST_DEATH_H_
#define MIR_TEST_DEATH_H_

#include <sys/resource.h>

namespace mir { namespace test {

inline void disable_core_dump()
{
    struct rlimit zeroes{0,0};
    setrlimit(RLIMIT_CORE, &zeroes);
}

} } // namespace mir::test

#endif /* MIR_TEST_DEATH_H_ */
