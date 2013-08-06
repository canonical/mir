/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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

class CompositeEventFilter : public EventFilter
{
public:
    virtual void append(std::shared_ptr<EventFilter> const& filter) = 0;
    virtual void prepend(std::shared_ptr<EventFilter> const& filter) = 0;
};

}
}

#endif /* MIR_INPUT_COMPOSITE_EVENT_FILTER_H_ */
