/*
 * Copyright © 2016-2020 Canonical Ltd.
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

#ifndef MIRAL_APPEND_EVENT_FILTER_H
#define MIRAL_APPEND_EVENT_FILTER_H

#include <functional>
#include <memory>

typedef struct MirEvent MirEvent;

namespace mir { class Server; }

namespace miral
{
/// \remark Since MirAL 3.0
class AppendEventFilter
{
public:
    AppendEventFilter(std::function<int(MirEvent const* event)> const& filter);

    void operator()(mir::Server& server);

private:
    class Filter;
    std::shared_ptr<Filter> const filter;
};
}

#endif //MIRAL_APPEND_EVENT_FILTER_H
