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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/input/filter.h"

#include <cassert>

namespace mi = mir::input;

void mi::NullFilter::accept(Event*) const
{
}

mi::ChainingFilter::ChainingFilter(std::shared_ptr<Filter> const& next_link)
    :
    next_link(next_link)
{
    assert(this->next_link.get());
}

void mi::ChainingFilter::accept(Event* e) const
{
    assert(e);
    next_link->accept(e);
}
