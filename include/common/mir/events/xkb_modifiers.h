/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_COMMON_XKB_MODIFIERS_H_
#define MIR_COMMON_XKB_MODIFIERS_H_

#include <cstdint>

struct MirXkbModifiers
{
    uint32_t depressed{0};
    uint32_t latched{0};
    uint32_t locked{0};
    uint32_t effective_layout{0};
};

inline auto operator==(MirXkbModifiers const& lhs, MirXkbModifiers const& rhs) -> bool
{
    return lhs.depressed == rhs.depressed &&
           lhs.latched == rhs.latched &&
           lhs.locked == rhs.locked &&
           lhs.effective_layout == rhs.effective_layout;
}

inline auto operator!=(MirXkbModifiers const& lhs, MirXkbModifiers const& rhs) -> bool
{
    return !(lhs == rhs);
}

#endif /* MIR_COMMON_XKB_MODIFIERS_H_ */
