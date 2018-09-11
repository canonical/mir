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
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
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

class OutputObserver
{
public:
    virtual void handle_configuration_change(graphics::DisplayConfiguration const& config) = 0;

    OutputObserver() = default;
    virtual ~OutputObserver() = default;
    OutputObserver(OutputObserver const&) = delete;
    OutputObserver& operator=(OutputObserver const&) = delete;
};

class MirDisplay
{
public:
    MirDisplay(
        std::shared_ptr<DisplayChanger> const& changer,
        std::shared_ptr<ObserverRegistrar<graphics::DisplayConfigurationObserver>>const& registrar);

    ~MirDisplay();

    void for_each_output(std::function<void(graphics::DisplayConfigurationOutput const&)> f) const;

    void register_interest(OutputObserver* observer);
    void unregister_interest(OutputObserver* observer);

private:

    struct Self;
    std::unique_ptr<Self> const self;
};
}
}

#endif //MIR_FRONTEND_DISPLAY_H_
