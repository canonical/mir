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

#include <mir/synchronised.h>
#include <mir/log.h>

#include <algorithm>
#include <atomic>
#include <boost/throw_exception.hpp>
#include <list>
#include <iterator>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>


namespace ml = mir::logging;

struct ml::Tag
{
    std::string const name;
    std::atomic<Severity> logging_severity;
    Tag const* parent;
};

namespace
{
auto construct_initial_tag_list() -> std::list<ml::Tag>
{
    decltype(construct_initial_tag_list()) list;
    list.emplace_back("base", ml::Severity::warning, nullptr);
    return list;
}

mir::Synchronised<std::list<ml::Tag>> known_tags{construct_initial_tag_list()}; //TICS !cppcoreguidelines-avoid-non-const-global-variables - This is the list of tags, shared within this module

/**
 * Ensure the core tags are registered as soon as possible.
 *
 * Global initialisation is unsequeneced between different translation units,
 * so we don't want to expose these globals directly, but by exposing the
 * functions that statically initialise these tags we can ensure that any static
 * initialisation dependencies are correctly ordered, and these hidden globals
 * ensure that the tags are registered by the time any function from this
 * translation unit is called.
 */
auto const& ensure_base_registered = ml::base();
auto const& ensure_input_registered = ml::input();
auto const& ensure_wayland_registered = ml::wayland();
auto const& ensure_graphics_registered = ml::graphics();
auto const& ensure_window_management_registered = ml::window_management();
}

auto ml::create_tag(Tag const& parent, std::string_view name) -> Tag const&
{
    if (!std::ranges::all_of(
        name,
        [](auto elem) {
            // Valid characters are 'a'-'z', '-' and '_'
            return (('a' <= elem) && (elem <= 'z')) || (elem == '-') || (elem == '_');
        }))
    {
        BOOST_THROW_EXCEPTION((
            std::invalid_argument{
                std::format("Tag name must consist of lowercase ASCII ('a'-'z'), '-', or '_' (found {})", name)}
        ));
    }

    auto locked_tags = known_tags.lock();
    locked_tags->emplace_back(std::string{name}, Severity::warning, &parent);
    return locked_tags->back();
}

auto ml::base() -> Tag const&
{
    static Tag const& base = known_tags.lock()->front();
    return base;
}

auto ml::input() -> Tag const&
{
    static Tag const& input = create_tag(base(), "input");
    return input;
}

auto ml::wayland() -> Tag const&
{
    static Tag const& wayland = create_tag(base(), "wayland");
    return wayland;
}

auto ml::graphics() -> Tag const&
{
    static Tag const& graphics = create_tag(base(), "graphics");
    return graphics;
}

auto ml::window_management() -> Tag const&
{
    static Tag const& window_management = create_tag(base(), "window-management");
    return window_management;
}

auto ml::parse_severity(std::string_view severity_name) -> ml::Severity
{
    if (severity_name == "critical")
    {
        return ml::Severity::critical;
    }
    else if (severity_name == "error")
    {
        return ml::Severity::error;
    }
    else if (severity_name == "warn" || severity_name == "warning")
    {
        return ml::Severity::warning;
    }
    else if (severity_name == "info" || severity_name == "informational")
    {
        return ml::Severity::informational;
    }
    else if (severity_name == "debug")
    {
        return ml::Severity::debug;

    }
    BOOST_THROW_EXCEPTION((std::out_of_range{std::format("Unrecognised severity: {}", severity_name)}));
}

namespace
{
auto lookup_tag_by_basename(decltype(known_tags)::Locked const& tag_list, std::string_view tag_name) -> ml::Tag&
{
    for (auto& tag : *tag_list)
    {
        if (tag.name == tag_name)
        {
            return tag;
        }
    }
    BOOST_THROW_EXCEPTION((std::out_of_range{std::format("Looking up nonexistent tag: {}", tag_name)}));
}

// Does the parent chain of `leaf` match the chain in `full_path`, ending at `base()`?
auto parent_chain_matches(ml::Tag const& leaf, std::span<std::string_view> full_path) -> bool
{
    auto current_tag = &leaf;

    // Drop the first tag (which must be "base"), then from the leaf backwards that each parent matches
    for (auto const& tag_name : full_path | std::views::drop(1) | std::views::reverse)
    {
        if (current_tag->name != tag_name)
        {
            // The path doesn't match, so this candidate is not the tag we're looking for.
            return false;
        }
        if (!current_tag->parent)
        {
            // This should be impossible, but it's cheap to *not* SIGSEGV here.
            BOOST_THROW_EXCEPTION((std::runtime_error{"Impossible tag chain: found a tag with no parent"}));
        }
        current_tag = current_tag->parent;
    }

    return current_tag == &ml::base(); // Ensure we ended at base()
}

auto lookup_tag_by_full_path(decltype(known_tags)::Locked const& tag_list, std::string_view full_path) -> ml::Tag&
{
    std::vector<std::string_view> tag_names;

    std::ranges::copy(
        full_path | std::views::split('/') | std::views::transform([](auto part) { return std::string_view(part); }),
        std::back_inserter(tag_names));

    for (auto& candidate : *tag_list | std::views::filter([name = tag_names.back()](auto const& tag) { return tag.name == name; }))
    {
        if (parent_chain_matches(candidate, tag_names))
        {
            return candidate;
        }
    }
    BOOST_THROW_EXCEPTION((std::out_of_range{std::format("Looking up nonexistent tag: {}", full_path)}));
}

auto lookup_tag(decltype(known_tags)::Locked const& tag_list, std::string_view request) -> ml::Tag&
{
    if (request.contains('/'))
    {
        return lookup_tag_by_full_path(tag_list, request);
    }
    else
    {
        return lookup_tag_by_basename(tag_list, request);
    }
}

template<typename Functor>
requires std::invocable<Functor, ml::Tag&>
void for_each_child(ml::Tag const& parent, decltype(known_tags)::Locked const& tag_list, Functor func)
{
    for (auto& tag : *tag_list)
    {
        if (tag.parent == &parent)
        {
            func(tag);
            for_each_child(tag, tag_list, func);
        }
    }
}

auto full_path_to_tag(ml::Tag const& tag) -> std::string
{
    if (!tag.parent)
    {
        return tag.name;
    }
    else
    {
        return full_path_to_tag(*tag.parent) + "/" + tag.name;
    }
}
}

auto ml::logging_enabled_for(Tag const& tag, Severity sev) -> bool
{
    return sev <= tag.logging_severity;
}

auto ml::list_known_tags() -> std::vector<std::string>
{
    decltype(ml::list_known_tags()) result;

    std::ranges::copy(
        *known_tags.lock() | std::views::transform([](auto const& tag) { return full_path_to_tag(tag); }),
        std::back_inserter(result));

    return result;
}

auto ml::tag::name(Tag const& tag) -> std::string_view
{
    return tag.name;
}

void ml::tag::set_severity(std::string_view name, Severity sev)
{
    auto locked_tags = known_tags.lock();
    auto& tag = lookup_tag(locked_tags, name);
    tag.logging_severity = sev;
    for_each_child(tag, locked_tags, [sev](ml::Tag& tag) { tag.logging_severity = sev; });
}

// GCC and Clang both ensure the switch is exhaustive.
// GCC, however, gets a "control reaches end of non-void function" warning without this
#ifndef __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
#endif

auto std::formatter<ml::Severity>::format(ml::Severity sev, std::format_context& ctx) const
    -> std::format_context::iterator
{
    auto const sev_to_str = [](ml::Severity sev)
        {
            switch (sev)
            {
                case ml::Severity::critical:
                    return "critical";
                case ml::Severity::error:
                    return "error";
                case ml::Severity::warning:
                    return "warning";
                case ml::Severity::informational:
                    return "info";
                case ml::Severity::debug:
                    return "debug";
            }
        };
    return std::formatter<std::string>::format(sev_to_str(sev), ctx);
}

#ifndef __clang__
#pragma GCC diagnostic pop
#endif
