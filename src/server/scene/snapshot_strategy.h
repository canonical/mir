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

#ifndef MIR_SCENE_SNAPSHOT_STRATEGY_H_
#define MIR_SCENE_SNAPSHOT_STRATEGY_H_

#include "mir/scene/snapshot.h"

#include <memory>

namespace mir
{
namespace compositor { class BufferStream; }
namespace scene
{

class SnapshotStrategy
{
public:
    virtual ~SnapshotStrategy() = default;

    virtual void take_snapshot_of(
        std::shared_ptr<compositor::BufferStream> const& surface_buffer_access,
        SnapshotCallback const& snapshot_taken) = 0;

protected:
    SnapshotStrategy() = default;
    SnapshotStrategy(SnapshotStrategy const&) = delete;
    SnapshotStrategy& operator=(SnapshotStrategy const&) = delete;
};
}
}

#endif /* MIR_SCENE_SNAPSHOT_STRATEGY_H_ */
