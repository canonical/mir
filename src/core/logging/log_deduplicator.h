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

#ifndef MIR_LOGGING_LOG_DEDUPLICATOR_H_
#define MIR_LOGGING_LOG_DEDUPLICATOR_H_

#include <mir/logging/logger.h>

#include <chrono>
#include <cstddef>
#include <format>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <tuple>
#include <utility>

namespace mir::logging::detail
{
class LogDeduplicator
{
public:
    using Clock = std::function<std::chrono::steady_clock::time_point()>;

    LogDeduplicator(std::chrono::steady_clock::duration window_size, Clock clock_source)
        : window{window_size},
          clock{std::move(clock_source)},
          last_pruned{clock()}
    {
    }

    auto filter(Severity severity, std::string const& component, std::string const& message)
        -> std::optional<std::string>
    {
        auto const now = clock();
        if (now - last_pruned >= window)
        {
            prune(now);
            last_pruned = now;
        }

        auto const [it, inserted] = entries.try_emplace(Key{severity, component, message}, Entry{now, 0});
        if (inserted)
            return message;

        auto& entry = it->second;
        if (now - entry.last_printed < window)
        {
            ++entry.suppressed;
            return std::nullopt;
        }

        auto const suppressed = entry.suppressed;
        entry = Entry{now, 0};
        if (suppressed == 0)
            return message;

        return std::format("{} (x{})", message, suppressed + 1);
    }

    auto size() const -> std::size_t
    {
        return entries.size();
    }

private:
    using Key = std::tuple<Severity, std::string, std::string>;

    struct Entry
    {
        std::chrono::steady_clock::time_point last_printed;
        std::size_t suppressed;
    };

    void prune(std::chrono::steady_clock::time_point now)
    {
        std::erase_if(entries, [&](auto const& item)
        {
            auto const& entry = item.second;
            return entry.suppressed == 0 && now - entry.last_printed >= window;
        });
    }

    std::chrono::steady_clock::duration window;
    Clock clock;
    std::chrono::steady_clock::time_point last_pruned;
    std::map<Key, Entry> entries;
};
}

#endif // MIR_LOGGING_LOG_DEDUPLICATOR_H_
