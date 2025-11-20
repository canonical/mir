/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * it under the terms of the GNU Lesser General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_UNWIND_HELPERS_H_
#define MIR_UNWIND_HELPERS_H_

#include <stdexcept>
#include <iostream>

namespace mir
{
template<typename Unwind>
class RevertIfUnwinding
{
public:
    template<typename Apply>
    RevertIfUnwinding(Apply && apply, Unwind&& unwind)
        : unwind{std::move(unwind)},
          initial_exception_count{std::uncaught_exceptions()}
    {
        apply();
    }

    RevertIfUnwinding(Unwind&& unwind)
        : unwind{std::move(unwind)},
          initial_exception_count{std::uncaught_exceptions()}
    {
    }

    RevertIfUnwinding(RevertIfUnwinding<Unwind>&& rhs)
        : unwind{std::move(rhs.unwind)},
          initial_exception_count{std::uncaught_exceptions()}
    {
        rhs.unwind = nullptr;
    }

    ~RevertIfUnwinding()
    {
        if (std::uncaught_exceptions() > initial_exception_count)
            unwind();
    }

private:
    RevertIfUnwinding(RevertIfUnwinding const&) = delete;
    RevertIfUnwinding& operator=(RevertIfUnwinding const&) = delete;

    Unwind unwind;
    int const initial_exception_count;
};

template<typename Apply, typename Revert>
inline auto try_but_revert_if_unwinding(Apply && apply, Revert && reverse) -> RevertIfUnwinding<Revert>
{
    return RevertIfUnwinding<Revert>{std::move(apply), std::move(reverse)};
}


template<typename Revert>
inline auto on_unwind(Revert && action) -> RevertIfUnwinding<Revert>
{
    return RevertIfUnwinding<Revert>{std::move(action)};
}

}

#endif
