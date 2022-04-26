/*
 * Copyright © 2018 Canonical Ltd.
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
 */

#ifndef MIR_FRONTEND_DISPLAY_H_
#define MIR_FRONTEND_DISPLAY_H_

#include <mir/observer_registrar.h>

#include <functional>
#include <memory>

namespace mir
{
namespace graphics
{
struct DisplayConfiguration;
struct DisplayConfigurationOutput;
class DisplayConfigurationObserver;
}
}

namespace mir
{
namespace frontend
{
class DisplayChanger;

class MirDisplay
{
public:
    MirDisplay(
        std::shared_ptr<DisplayChanger> const& changer,
        std::shared_ptr<ObserverRegistrar<graphics::DisplayConfigurationObserver>>const& registrar);

    ~MirDisplay();

    void for_each_output(std::function<void(graphics::DisplayConfigurationOutput const&)> f) const;

    void register_interest(std::weak_ptr<graphics::DisplayConfigurationObserver> const& observer, Executor& executor);
    void unregister_interest(graphics::DisplayConfigurationObserver const& observer);

private:

    struct Self;
    std::unique_ptr<Self> const self;
};
}
}

#endif //MIR_FRONTEND_DISPLAY_H_
