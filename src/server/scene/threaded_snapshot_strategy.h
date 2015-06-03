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

#ifndef MIR_SCENE_THREADED_SNAPSHOT_STRATEGY_H_
#define MIR_SCENE_THREADED_SNAPSHOT_STRATEGY_H_

#include "snapshot_strategy.h"

#include <memory>
#include <thread>
#include <functional>

namespace mir
{
namespace scene
{
class PixelBuffer;
class SnapshottingFunctor;

class ThreadedSnapshotStrategy : public SnapshotStrategy
{
public:
    ThreadedSnapshotStrategy(std::shared_ptr<PixelBuffer> const& pixels);
    ~ThreadedSnapshotStrategy() noexcept;

    void take_snapshot_of(
        std::shared_ptr<compositor::BufferStream> const& surface_buffer_access,
        SnapshotCallback const& snapshot_taken);

private:
    std::shared_ptr<PixelBuffer> const pixels;
    std::unique_ptr<SnapshottingFunctor> functor;
    std::thread thread;
};

}
}

#endif /* MIR_SCENE_THREADED_SNAPSHOT_STRATEGY_H_ */
