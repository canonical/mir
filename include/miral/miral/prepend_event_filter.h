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
/// Prepends an event filter to the Mir server event pipeline.
///
/// Filters are processed in order. This filter is prepended **before** any existing filters,
/// including those provided by #miral::WindowManagementPolicy.
///
/// /// An event is passed to this filter only if no earlier filter has already handled it.
///
/// \sa AppendKeyboardEventFilter - a specialized event filter which only filters keyboard events
/// \sa AppendEventFilter - append an event filter
class PrependEventFilter
{
public:
    /// Construct a new event filter wrapper using the provided \p filter function.
    ///
    /// \param filter A function that returns `true` if it handled the event.
    ///               Returning `true` prevents later filters from seeing the event.
    explicit PrependEventFilter(std::function<bool(MirEvent const* event)> const& filter);

    void operator()(mir::Server& server);

private:
    class Filter;
    std::shared_ptr<Filter> const filter;
};
}

#endif //MIRAL_PREPEND_EVENT_FILTER_H
