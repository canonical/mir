/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#ifndef MIR_INPUT_GRAB_FILTER_H_
#define MIR_INPUT_GRAB_FILTER_H_

#include "mir/input/dispatcher.h"

#include <memory>
#include <list>

namespace mir
{

namespace input
{

class Event;

class GrabHandle;
    
class GrabFilter : public ChainingFilter
{
 public:
	//using ChainingFilter::ChainingFilter;
	GrabFilter(std::shared_ptr<Filter> const& next_link) : ChainingFilter(next_link) {}

    void accept(Event* e) const;

    GrabHandle push_grab(std::shared_ptr<EventHandler> const& event_handler);
	void release_grab(GrabHandle const& grab_handle);

 private:
	std::list<std::shared_ptr<EventHandler>> grabs;
};

class GrabHandle
{
	friend class GrabFilter;

	typedef std::list<std::shared_ptr<EventHandler>>::iterator Handle;
	GrabHandle(Handle i) : i(i) {}
	GrabHandle() = default;
	Handle i;
};
}
}

#endif // MIR_INPUT_GRAB_FILTER_H_
