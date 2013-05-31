/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_CACHED_PTR_H_
#define MIR_CACHED_PTR_H_

#include <functional>
#include <memory>

namespace mir
{
template<typename Type>
class CachedPtr
{
    std::weak_ptr<Type> cache;
    CachedPtr(CachedPtr const&) = delete;
    CachedPtr& operator=(CachedPtr const&) = delete;
public:
    CachedPtr() = default;

    std::shared_ptr<Type> operator()(std::function<std::shared_ptr<Type>()> make)
    {
        auto result = cache.lock();
        if (!result)
        {
                cache = result = make();
        }
        return result;
    }
};
} // namespace mir

#endif // MIR_CACHED_PTR_H_
