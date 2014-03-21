/*
 * Copyright © 2013-2014 Canonical Ltd.
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

#ifndef MIR_SCENE_BASIC_SURFACE_FACTORY_H_
#define MIR_SCENE_BASIC_SURFACE_FACTORY_H_

#include "mir/frontend/surface_id.h"
#include "mir/shell/surface_creation_parameters.h"
#include <memory>
#include <functional>

namespace mir
{
namespace frontend { class EventSink; }
namespace shell { class SurfaceConfigurator; }
namespace scene
{

class Surface;
class BasicSurfaceFactory
{
public:
    BasicSurfaceFactory() = default;
    virtual ~BasicSurfaceFactory() = default;

    virtual std::shared_ptr<Surface> create_surface(
        frontend::SurfaceId id,
        shell::SurfaceCreationParameters const& params,
        std::shared_ptr<frontend::EventSink> const& event_sink,
        std::shared_ptr<shell::SurfaceConfigurator> const& configurator) = 0;
private:
    BasicSurfaceFactory(const BasicSurfaceFactory&) = delete;
    BasicSurfaceFactory& operator=(const BasicSurfaceFactory&) = delete;
};

}
}

#endif /* MIR_SCENE_BASIC_SURFACE_FACTORY_H_ */
