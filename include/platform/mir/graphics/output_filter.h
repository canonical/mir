/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef MIR_GRAPHICS_OUTPUT_FILTER_H_
#define MIR_GRAPHICS_OUTPUT_FILTER_H_

#include "mir_toolkit/common.h"

namespace mir
{
namespace graphics
{
class OutputFilter
{
public:
    virtual MirOutputFilter filter() = 0;
    virtual void filter(MirOutputFilter new_filter) = 0;
protected:
    OutputFilter() = default;
    virtual ~OutputFilter() = default;
    OutputFilter(OutputFilter const&) = delete;
    OutputFilter& operator=(OutputFilter const&) = delete;
};
}
}


#endif /* MIR_GRAPHICS_OUTPUT_FILTER_H_ */
