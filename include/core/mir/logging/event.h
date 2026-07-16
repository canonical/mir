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

#ifndef MIR_LOGGING_EVENT_H_
#define MIR_LOGGING_EVENT_H_

#include <mir/logging/tag.h>

#include <memory>
#include <string_view>
#include <source_location>

namespace mir::logging
{
/**
 * A representation of a point-in-time event for logging
 */
class Event
{
public:
    /**
     * Create an Event
     *
     * \warning The `tags` and `message` parameters are non-owning views; an
     * Event should almost always be directly constructed during a function call
     * to Logger::log().
     */
    Event(
        Severity sev,
        Tags tags,
        std::string_view message,
        std::source_location location = std::source_location::current());

    /**
     * Transitional Event constructor for call-sites that have not yet migrated to Tags
     *
     * These Events are all categorised in a catch-all "uncategorised" tag;
     * the `category` included here is added to the message
     */
    Event(
        Severity sev,
        std::string_view component,
        std::string_view message,
        std::source_location location = std::source_location::current());

    auto severity() const -> Severity;
    auto tags() const -> Tags;
    auto message() const -> std::string_view;
    auto location() const -> std::source_location;

    /**
     * Should this Event be logged given the current logging configuration?
     */
    auto should_log() const -> bool;

    Event(Event const&) = delete;
    Event(Event&&) = delete;
    auto operator=(Event const&) -> Event& = delete;
    auto operator=(Event&&) -> Event& = delete;

private:
    class Impl;
    // Impl ptr has explicit deleter to allow us to do memory shenanagins for optimisation.
    std::unique_ptr<Impl, void (*)(Impl*)> const impl;
};
}

#endif
