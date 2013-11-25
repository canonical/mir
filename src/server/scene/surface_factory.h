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

#ifndef MIR_SCENE_SURFACE_FACTORY_H_
#define MIR_SCENE_SURFACE_FACTORY_H_

#include "mir/shell/surface_creation_parameters.h"
#include <memory>
#include <functional>

namespace mir
{
namespace scene
{

class BasicSurface;
class SurfaceFactory
{
public:
    SurfaceFactory() {};
    virtual ~SurfaceFactory() = default;

    virtual std::shared_ptr<BasicSurface> create_surface(
        shell::SurfaceCreationParameters const&, std::function<void()> const&) = 0;
private:
    SurfaceFactory(const SurfaceFactory&) = delete;
    SurfaceFactory& operator=(const SurfaceFactory&) = delete;
};

}
}

#endif /* MIR_SCENE_SURFACE_FACTORY_H_ */
