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

#include <algorithm>
#include <ranges>

namespace ml = mir::logging;

class ml::Event::Impl
{
public:
    Impl(
        Severity sev,
        Tags tags,
        std::source_location loc)
        : sev_{sev},
          tags_{tags},
          location_{loc}
    {
    }

    virtual ~Impl() = default;

    auto severity() const -> Severity
    {
        return sev_;
    }

    auto tags() const -> Tags
    {
        return tags_;
    }

    virtual auto message() const -> std::string = 0;

    auto location() const -> std::source_location
    {
        return location_;
    }

private:
    Severity const sev_;
    Tags const tags_;
    std::source_location const location_;
};

class DeferredFormattingImpl : public ml::Event::Impl
{
public:
    DeferredFormattingImpl(
        ml::Severity sev,
        ml::Tags tags,
        std::string_view fmt,
        std::format_args args,
        std::source_location loc)
        : Impl(sev, tags, loc),
          fmt{fmt},
          args{args}
    {
    }

    auto message() const -> std::string override
    {
        return std::vformat(fmt, args);
    }
private:
    std::string_view const fmt;
    std::format_args const args;
};

class ImplWithComponentAndMessage : public ml::Event::Impl
{
public:
    ImplWithComponentAndMessage(
        ml::Severity sev,
        std::string_view component,
        std::string_view message,
        std::source_location loc)
        : Impl(sev, std::span{&uncategorised_ref, 1}, loc),
          message_{message},
          component{component}
    {
    }

    auto message() const -> std::string override
    {
        return std::format("{}: {}", component, message_);
    }

private:
    static std::reference_wrapper<ml::Tag const> const uncategorised_ref;
    std::string const message_;
    std::string const component;
};

static auto const& uncategorised_tag = ml::create_tag(ml::base(), "uncategorised");
std::reference_wrapper<ml::Tag const> const ImplWithComponentAndMessage::uncategorised_ref = std::cref(uncategorised_tag);

ml::Event::Event(
    Severity sev,
    Tags tags,
    std::string_view fmt,
    std::format_args args,
    std::source_location location)
{
    static_assert(sizeof(DeferredFormattingImpl) < sizeof(decltype(storage)));
    /* Sadly we can't do `alignof(storage)`; we need to manually keep the alignas
     * on the declaration of storage and the alignof check here in sync.
     */
    static_assert(alignof(std::max_align_t) >= alignof(DeferredFormattingImpl));

    new(storage.data()) DeferredFormattingImpl{sev, tags, fmt, args, location};
}

ml::Event::Event(
    Severity sev,
    std::string_view component,
    std::string_view message,
    std::source_location location)
{
    static_assert(sizeof(ImplWithComponentAndMessage) < sizeof(decltype(storage)));
    /* Sadly we can't do `alignof(storage)`; we need to manually keep the alignas
     * on the declaration of storage and the alignof check here in sync.
     */
    static_assert(alignof(std::max_align_t) >= alignof(ImplWithComponentAndMessage));

    new(storage.data()) ImplWithComponentAndMessage{sev, component, message, location};
}

ml::Event::~Event()
{
    // We have our own storage, so need to call the destructor manually ourselves.
    impl()->~Impl();
}


auto ml::Event::severity() const -> Severity
{
    return impl()->severity();
}

auto ml::Event::tags() const -> Tags
{
    return impl()->tags();
}

auto ml::Event::message() const -> std::string
{
    return impl()->message();
}

auto ml::Event::location() const -> std::source_location
{
    return impl()->location();
}

auto ml::Event::should_log() const -> bool
{
    auto const sev = impl()->severity();
    return
        std::ranges::any_of(impl()->tags(), [sev](Tag const& tag) { return ml::logging_enabled_for(tag, sev); });
}

auto ml::Event::impl() const -> Impl const*
{
    return std::launder(reinterpret_cast<Impl const*>(storage.data()));
}
