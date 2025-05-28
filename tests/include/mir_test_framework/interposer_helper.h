/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <memory>
#include <functional>
#include <optional>
#include <list>
#include <atomic>
#include <mutex>

namespace mir_test_framework
{
using InterposerHandle = std::unique_ptr<void, void(*)(void*)>;

template<typename Returns, typename... Args>
class InterposerHandlers
{
public:
    using Handler = std::function<std::optional<Returns>(Args...)>;
  
    static auto add(Handler handler) -> InterposerHandle
    {
        handlers_added.store(true);
        auto& me = instance();
        std::lock_guard lock{me.mutex};
        auto iterator = me.handlers.emplace(me.handlers.begin(), std::move(handler));
        auto remove_callback = [](void* iterator)
            {
                auto to_remove = static_cast<typename std::list<Handler>::iterator*>(iterator);
                remove(to_remove);
            };
        return InterposerHandle{
            static_cast<void*>(new typename std::list<Handler>::iterator{iterator}),
            remove_callback};
    }

    static auto run(Args ...args) -> std::optional<Returns>
    {
        if (handlers_added)
        {
            auto& me = instance();
            std::lock_guard lock{me.mutex};
            for (auto const& handler: me.handlers)
            {
                if (auto val = handler(args...))
                {
                    return val;
                }
            }
        }
        return std::nullopt;
    }

private:
    // We want to ensure that run() doesn't call instance() too early (e.g. during coverage setup)
    // as that leads to destruction order issues. (https://github.com/canonical/mir/issues/2387)
    static std::atomic<bool> handlers_added;

    static auto instance() -> InterposerHandlers&
    {
        // static local so we don't have to worry about initialization order
        static InterposerHandlers handlers;
        return handlers;
    }

    static void remove(typename std::list<Handler>::iterator* to_remove)
    {
        auto& me = instance();
        std::lock_guard lock{me.mutex};
        me.handlers.erase(*to_remove);
        delete to_remove;
    }

    std::mutex mutex;
    std::list<Handler> handlers;
};

template<typename Returns, typename... Args>
std::atomic<bool> InterposerHandlers<Returns, Args...>::handlers_added{false};
}