/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_SCENE_SURFACE_ALLOCATOR_H_
#define MIR_SCENE_SURFACE_ALLOCATOR_H_

#include "surface_factory.h"

namespace mir
{
namespace input
{
class InputChannelFactory;
}
namespace scene
{
class BufferStreamFactory;
class SceneReport;

class SurfaceAllocator : public SurfaceFactory
{
public:
    SurfaceAllocator(std::shared_ptr<BufferStreamFactory> const& bb_factory,
                     std::shared_ptr<input::InputChannelFactory> const& input_factory,
                     std::shared_ptr<SceneReport> const& report);

    std::shared_ptr<BasicSurface> create_surface(shell::SurfaceCreationParameters const&, std::function<void()> const&);

private:
    std::shared_ptr<BufferStreamFactory> const buffer_stream_factory;
    std::shared_ptr<input::InputChannelFactory> const input_factory;
    std::shared_ptr<SceneReport> const report;
};

}
}

#endif /* MIR_SCENE_SURFACE_ALLOCATOR_H_ */
