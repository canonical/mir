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

#include <type_traits>
#include <memory>

namespace mir
{

/**
 * A minimal version of std::unique_ptr that is guaranteed to be a StandardLayoutType
 */
template<typename T>
class StdLayoutUPtr
{
public:
    StdLayoutUPtr() = default;

    StdLayoutUPtr(T* value) noexcept
        : value{value}
    {
    }

    StdLayoutUPtr(StdLayoutUPtr<T>&& from) noexcept
        : value{from.value}
    {
        from.value = nullptr;
    }

    StdLayoutUPtr(std::unique_ptr<T>&& from) noexcept
        : value{from.release()}
    {
    }

    ~StdLayoutUPtr() noexcept
    {
        delete value;
    }

    auto operator=(StdLayoutUPtr<T>&& from) noexcept -> StdLayoutUPtr<T>&
    {
        delete value;
        value = from.value;
        from.value = nullptr;
        return *this;
    }

    auto get() const noexcept -> T*
    {
        return value;
    }

    auto operator*() const noexcept -> typename std::add_lvalue_reference<T>::type
    {
        return *value;
    }

    auto operator->() const noexcept -> T*
    {
        return value;
    }

    explicit operator bool() const noexcept
    {
        return value != nullptr;
    }

    void reset() noexcept
    {
        delete value;
        value = nullptr;
    }
private:
    T* value{nullptr};
};

}

static_assert(
    std::is_standard_layout<mir::StdLayoutUPtr<int>>::value,
    "StdLayoutUPtr<int> is meant to be standard layout, damnit!");
