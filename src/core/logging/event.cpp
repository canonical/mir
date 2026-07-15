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

#include <ranges>

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

    class ImplWithAllocatedMessage;
private:
    Severity const sev_;
    Tags const tags_;
    std::string_view const message_;
    std::source_location const location_;
};

static auto const& uncategorised_tag = ml::create_tag(ml::base(), "uncategorised");

class ml::Event::Impl::ImplWithAllocatedMessage : public Impl
{
public:
    ImplWithAllocatedMessage(
        Severity sev,
        std::unique_ptr<char const[]> message,
        std::source_location loc)
        : Impl(sev, std::span{&uncategorised_ref, 1}, std::string_view{message.get()}, loc),
          message_storage{std::move(message)}
    {
    }
private:
    static std::reference_wrapper<Tag const> const uncategorised_ref;
    std::unique_ptr<char const[]> const message_storage;
};

std::reference_wrapper<ml::Tag const> const ml::Event::Impl::ImplWithAllocatedMessage::uncategorised_ref = std::cref(uncategorised_tag);

ml::Event::Event(
    Severity sev,
    Tags tags,
    std::string_view message,
    std::source_location location)
    : impl{new Impl{sev, tags, message, location}, [](Impl* p) { delete p; }}
{
}

namespace
{
auto create_message_string(std::string_view component, std::string_view message) -> std::unique_ptr<char const[]>
{
    auto const combined_message = std::views::concat(component, std::string_view{": "}, message);
    auto storage = std::make_unique<char[]>(combined_message.size() + 1);

    std::ranges::copy(combined_message, storage.get());

    return storage;
}
}

ml::Event::Event(
    Severity sev,
    std::string_view component,
    std::string_view message,
    std::source_location location)
    : impl{
        new Impl::ImplWithAllocatedMessage{sev, create_message_string(component, message), location},
        [](Impl* p) { delete static_cast<Impl::ImplWithAllocatedMessage*>(p); }}
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
