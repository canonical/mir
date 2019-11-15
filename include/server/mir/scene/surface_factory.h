/*
 * Copyright © 2013-2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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

#ifndef MIR_SCENE_SURFACE_FACTORY_H_
#define MIR_SCENE_SURFACE_FACTORY_H_

#include "mir/scene/surface_creation_parameters.h"
#include <memory>
#include <list>

namespace mir
{
namespace compositor { class BufferStream; }
namespace scene
{
class Surface;
class Session;
class StreamInfo;

class SurfaceFactory
{
public:
    SurfaceFactory() = default;
    virtual ~SurfaceFactory() = default;

    virtual std::shared_ptr<Surface> create_surface(
        std::shared_ptr<Session> const& session,
        std::list<scene::StreamInfo> const& streams,
        SurfaceCreationParameters const& params) = 0;

private:
    SurfaceFactory(const SurfaceFactory&) = delete;
    SurfaceFactory& operator=(const SurfaceFactory&) = delete;
};

}
}

#endif /* MIR_SCENE_SURFACE_FACTORY_H_ */
