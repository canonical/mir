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
 * Authored By: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_SURFACES_SNAPSHOT_H_
#define MIR_SURFACES_SNAPSHOT_H_

#include "mir/geometry/size.h"
#include "mir/geometry/dimensions.h"

#include <functional>

namespace mir
{
namespace shell
{

struct Snapshot
{
    geometry::Size size;
    geometry::Stride stride;
    void const* pixels;
};

typedef std::function<void(Snapshot const&)> SnapshotCallback;

}
}

#endif /* MIR_SURFACES_SNAPSHOT_H_ */
