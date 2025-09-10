/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
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

#ifndef MIRAL_PREPEND_EVENT_FILTER_H
#define MIRAL_PREPEND_EVENT_FILTER_H

#include <functional>
#include <memory>

typedef struct MirEvent MirEvent;

namespace mir { class Server; }

namespace miral
{
class PrependEventFilter
{
public:
    /// Prepend an event filter (before any existing filters, including the window manager).
    /// The supplied filter should return true if and only if it handles the event as filters
    /// later in the list will not be called.
    explicit PrependEventFilter(std::function<bool(MirEvent const* event)> const& filter);

    void operator()(mir::Server& server);

private:
    class Filter;
    std::shared_ptr<Filter> const filter;
};
}

#endif //MIRAL_PREPEND_EVENT_FILTER_H
