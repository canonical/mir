/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_DISPLAY_CONFIGURATION_MULTIPLEXER_H_
#define MIR_DISPLAY_CONFIGURATION_MULTIPLEXER_H_

#include "mir/observer_registrar.h"
#include "mir/observer_multiplexer.h"
#include "mir/graphics/display_configuration_observer.h"

namespace mir
{
namespace graphics
{
class DisplayConfigurationObserverMultiplexer : public ObserverMultiplexer<DisplayConfigurationObserver>
{
public:
    void initial_configuration(DisplayConfiguration const& config) override;

    void configuration_applied(DisplayConfiguration const& config) override;

    void configuration_failed(DisplayConfiguration const& attempted, std::exception const& error) override;
};

}
}

#endif //MIR_DISPLAY_CONFIGURATION_MULTIPLEXER_H_
