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

#include <mir/logging/tag.h>
#include <mir/logging/event.h>

namespace ml = mir::logging;

class ml::Event::Impl
{
public:
    Impl(Severity sev, Tags tags, std::string_view message, std::source_location loc)
        : sev_{sev},
          tags_{tags},
          message_{message},
          location_{loc}
    {
    }

    auto severity() const -> Severity
    {
        return sev_;
    }

    auto tags() const -> Tags
    {
        return tags_;
    }

    auto message() const -> std::string_view
    {
        return message_;
    }

    auto location() const -> std::source_location
    {
        return location_;
    }

private:
    Severity const sev_;
    Tags const tags_;
    std::string_view const message_;
    std::source_location const location_;
};

ml::Event::Event(
    Severity sev,
    Tags tags,
    std::string_view message,
    std::source_location location)
    : impl{new Impl{sev, tags, message, location}, [](Impl* p) { delete p; }}
{
}

auto ml::Event::severity() const -> Severity
{
    return impl->severity();
}

auto ml::Event::tags() const -> Tags
{
    return impl->tags();
}

auto ml::Event::message() const -> std::string_view
{
    return impl->message();
}

auto ml::Event::location() const -> std::source_location
{
    return impl->location();
}
