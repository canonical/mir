/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_DEFAULT_INITIAL_RENDER_MANAGER_H
#define MIR_DEFAULT_INITIAL_RENDER_MANAGER_H

#include "mir/graphics/initial_render.h"

#include <memory>

namespace mir
{

namespace time
{
class Clock;
class Alarm;
class AlarmFactory;
}

namespace input
{
class Scene;
}

namespace  graphics
{

class DefaultInitialRenderManager : public InitialRenderManager
{
public:
    DefaultInitialRenderManager(
        std::shared_ptr<time::Clock>& clock,
        time::AlarmFactory& alarm_factory,
        std::shared_ptr<input::Scene>& scene);
    ~DefaultInitialRenderManager();
    void add_initial_render(std::shared_ptr<InitialRender> const&) override;

private:
    std::shared_ptr<time::Clock> const& clock;
    time::AlarmFactory& alarm_factory;
    std::shared_ptr<input::Scene>& scene;
    std::vector<std::shared_ptr<InitialRender>> renderable_list;
};

}
}


#endif //MIR_DEFAULT_INITIAL_RENDER_MANAGER_H
