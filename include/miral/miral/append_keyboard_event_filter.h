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

#ifndef MIRAL_APPEND_KEYBOARD_EVENT_FILTER_H
#define MIRAL_APPEND_KEYBOARD_EVENT_FILTER_H

#include <functional>
#include <memory>

struct MirKeyboardEvent;

namespace mir { class Server; }

namespace miral
{
/// Appends a keyboard event filter to the Mir server event pipeline.
///
/// Filters are processed in order. This filter is appended **after** any existing filters,
/// including those provided by `miral::WindowManagementPolicy`.
///
/// An event is passed to this filter only if no earlier filter has already handled it.
///
/// \sa miral::WindowManagementPolicy - the keyboard event handler for the window management policy.
/// \sa AppendEventFilter - add a filter for any event, not just keyboards
///
/// \remark Since MirAL 5.5
class AppendKeyboardEventFilter
{
public:
    // Constructs a new keyboard filter wrapper using the provided `filter` function.
    ///
    /// \param filter A function that returns `true` if it handled the keyboard event.
    ///               Returning `true` prevents later filters from seeing the event.
    explicit AppendKeyboardEventFilter(std::function<bool(MirKeyboardEvent const* event)> const& filter);

    void operator()(mir::Server& server);

private:
    class Filter;
    std::shared_ptr<Filter> const filter;
};
}

#endif //MIRAL_APPEND_KEYBOARD_EVENT_FILTER_H
