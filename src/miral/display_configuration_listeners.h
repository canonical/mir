/*
 * Copyright © 2017 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
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

#ifndef MIRAL_DISPLAY_CONFIGURATION_OBSERVERS_H
#define MIRAL_DISPLAY_CONFIGURATION_OBSERVERS_H

#include "miral/output.h"

#include <mir/graphics/display_configuration_observer.h>

#include <mutex>
#include <vector>

namespace miral
{
class ActiveOutputsListener;

class DisplayConfigurationListeners :  public mir::graphics::DisplayConfigurationObserver
{
public:
    void add_listener(ActiveOutputsListener* listener);

    void delete_listener(ActiveOutputsListener* listener);

    void process_active_outputs(std::function<void(std::vector<Output> const& outputs)> const& functor) const;

private:
    void initial_configuration(std::shared_ptr<mir::graphics::DisplayConfiguration const> const& configuration) override;
    void configuration_applied(std::shared_ptr<mir::graphics::DisplayConfiguration const> const& config) override;

    void configuration_failed(
        std::shared_ptr<mir::graphics::DisplayConfiguration const> const&,
        std::exception const&) override;

    void catastrophic_configuration_error(
        std::shared_ptr<mir::graphics::DisplayConfiguration const> const&,
        std::exception const&) override;

    void base_configuration_updated(std::shared_ptr<mir::graphics::DisplayConfiguration const> const&) override;

    void session_configuration_applied(std::shared_ptr<mir::scene::Session> const&,
                                       std::shared_ptr<mir::graphics::DisplayConfiguration> const&) override;

    void session_configuration_removed(std::shared_ptr<mir::scene::Session> const&) override;

    std::mutex mutable mutex;
    std::vector<ActiveOutputsListener*> active_output_listeners;
    std::vector<Output> active_outputs;
};
}

#endif //MIRAL_DISPLAY_CONFIGURATION_OBSERVERS_H
