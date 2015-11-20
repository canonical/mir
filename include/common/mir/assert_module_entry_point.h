/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_ASSERT_MODULE_ENTRY_POINT_H_
#define MIR_ASSERT_MODULE_ENTRY_POINT_H_

#include <type_traits>

namespace mir
{
template<typename ReferenceTypename, typename EntryPoint>
void assert_entry_point_signature(EntryPoint)
{
    static_assert(std::is_same<EntryPoint, ReferenceTypename>::value,
                  "Signature of platform entry point does not match.");
}
}

#endif
