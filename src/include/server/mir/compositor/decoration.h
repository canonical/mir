/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#ifndef MIR_COMPOSITOR_DECORATION_H_
#define MIR_COMPOSITOR_DECORATION_H_

#include <string>

namespace mir { namespace compositor {

struct Decoration
{
    // TODO enum class Type {none, window} type;  ...
    std::string name;
};

} } // namespace mir::compositor

#endif // MIR_COMPOSITOR_DECORATION_H_
