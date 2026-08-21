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

#ifndef MIR_SHELL_MANAGED_WINDOW_MEMBERSHIP_H_
#define MIR_SHELL_MANAGED_WINDOW_MEMBERSHIP_H_

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <utility>
#include <vector>

namespace mir
{
namespace scene { class Surface; }
namespace shell
{
class ManagedWindowMembership
{
public:
    virtual auto contains(std::shared_ptr<scene::Surface> const& surface) const -> bool = 0;

    virtual ~ManagedWindowMembership() = default;
    ManagedWindowMembership() = default;
    ManagedWindowMembership(ManagedWindowMembership const&) = delete;
    ManagedWindowMembership& operator=(ManagedWindowMembership const&) = delete;
};

/// A synchronised window registry whose values are owned by the window manager.
template<typename Value>
class ManagedWindowRegistry : public ManagedWindowMembership
{
public:
    using Map = std::map<std::weak_ptr<scene::Surface>, Value, std::owner_less<std::weak_ptr<scene::Surface>>>;

    auto contains(std::shared_ptr<scene::Surface> const& surface) const -> bool override
    {
        std::lock_guard lock{mutex};
        return windows.contains(surface);
    }

    template<typename... Args>
    auto emplace(std::shared_ptr<scene::Surface> const& surface, Args&&... args) -> Value&
    {
        std::lock_guard lock{mutex};
        return windows.emplace(surface, std::forward<Args>(args)...).first->second;
    }

    template<typename ValueLike>
    void insert_or_assign(std::shared_ptr<scene::Surface> const& surface, ValueLike&& value)
    {
        std::lock_guard lock{mutex};
        windows.insert_or_assign(surface, std::forward<ValueLike>(value));
    }

    template<typename Key>
    void erase(Key const& key)
    {
        std::lock_guard lock{mutex};
        windows.erase(key);
    }

    auto find(std::weak_ptr<scene::Surface> const& surface) -> std::optional<std::reference_wrapper<Value>>
    {
        std::lock_guard lock{mutex};
        if (auto found = windows.find(surface); found != windows.end())
            return found->second;

        return {};
    }

    auto snapshot() const -> std::vector<std::pair<std::weak_ptr<scene::Surface>, Value>>
    {
        std::lock_guard lock{mutex};
        return {windows.begin(), windows.end()};
    }
private:
    mutable std::mutex mutex;
    Map windows;
};
}
}

#endif /* MIR_SHELL_MANAGED_WINDOW_MEMBERSHIP_H_ */
