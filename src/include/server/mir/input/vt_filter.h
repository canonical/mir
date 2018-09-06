/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Robert Ancell <robert.ancell@canonical.com>
 */

#ifndef MIR_INPUT_VT_FILTER_H_
#define MIR_INPUT_VT_FILTER_H_

#include "mir/input/event_filter.h"

#include <memory>

namespace mir
{
class VTSwitcher;

namespace input
{

class VTFilter : public EventFilter
{
public:
    VTFilter(std::unique_ptr<VTSwitcher> switcher);

    bool handle(MirEvent const& event) override;

private:
    std::unique_ptr<VTSwitcher> const switcher;
};

}
}

#endif // MIR_INPUT_VT_FILTER_H_
