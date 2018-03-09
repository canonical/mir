/*
 * Copyright © 2018 Canonical Ltd.
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
 * Authored by: William Wold <william.wold@canonical.com>
 */

#ifndef MIR_FRONTEND_DOUBLE_BUFFERED_H
#define MIR_FRONTEND_DOUBLE_BUFFERED_H

#include <experimental/optional>

namespace mir
{
namespace frontend
{
template <typename T>
class DoubleBuffered
{
public:
    DoubleBuffered() {}

    DoubleBuffered(T value)
    {
        current = value;
    }

    void operator=(T const& value)
    {
        pending = value;
    }

    operator T const&() const
    {
        return current;
    }

    T const* operator->() const
    {
        return &current;
    }

    T const& operator*() const
    {
        return current;
    }

    std::experimental::optional<T> const& get_pending() const
    {
        return pending;
    }

    void commit()
    {
        if (pending)
        {
            current = pending.value();
            pending = std::experimental::nullopt;
        }
    }

private:
    T current;
    std::experimental::optional<T> pending;
};
}
}

#endif // MIR_FRONTEND_DOUBLE_BUFFERED_H
