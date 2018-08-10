/*
 * Copyright © 2013 Canonical Ltd.
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
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_INPUT_COMPOSITE_EVENT_FILTER_H_
#define MIR_INPUT_COMPOSITE_EVENT_FILTER_H_

#include "mir/input/event_filter.h"

#include <memory>

namespace mir
{
namespace input
{

/**
 * Apply a chain of filters to events
 *
 * This composite filter offers the event to each filter in turn until one consumes it.
 */
class CompositeEventFilter : public EventFilter
{
public:
    /**
     * Add a filter to the end of the chain; it will be offered each event after any existing filters.
     *
     * The filter will be removed when the lifetime of the underlying object ends.
     *
     * \param [in] filter   Filter to append to chain.
     */
    virtual void append(std::weak_ptr<EventFilter> const& filter) = 0;
    /**
     * Add a filter to the beginning of the chain; it will be offered each event before any existing filters.
     *
     * The filter will be removed when the lifetime of the underlying object ends.
     *
     * \param [in] filter   Filter to prepend to chain.
     */
    virtual void prepend(std::weak_ptr<EventFilter> const& filter) = 0;
};

}
}

#endif /* MIR_INPUT_COMPOSITE_EVENT_FILTER_H_ */
