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

#ifndef MIR_RECENT_ITEMS_H_
#define MIR_RECENT_ITEMS_H_

#include <algorithm>
#include <array>
#include <cstddef>
#include <optional>

namespace mir
{
/// A fixed-capacity circular buffer of recently added items.
template<typename T, std::size_t Capacity>
    requires(Capacity > 0)
class RecentItems
{
public:
    static constexpr auto capacity = Capacity;

    void add(T const& item)
    {
        *next = item;

        if (++next == items.end())
            next = items.begin();
    }

    auto contains(T const& item) const -> bool
    {
        return std::ranges::any_of(items, [&item](auto const& current)
            {
                return current && *current == item;
            });
    }

private:
    std::array<std::optional<T>, Capacity> items;
    decltype(items)::iterator next{items.begin()};
};
}


#endif // MIR_RECENT_ITEMS_H_
