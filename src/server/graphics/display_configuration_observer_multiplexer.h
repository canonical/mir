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

#ifndef MIR_DISPLAY_CONFIGURATION_MULTIPLEXER_H_
#define MIR_DISPLAY_CONFIGURATION_MULTIPLEXER_H_

#include "mir/observer_registrar.h"
#include "mir/observer_multiplexer.h"
#include "mir/graphics/display_configuration_observer.h"

#include <mutex>

namespace mir
{
namespace frontend
{
class Session;
}
namespace graphics
{
class DisplayConfigurationObserverMultiplexer : public ObserverMultiplexer<DisplayConfigurationObserver>
{
public:
    DisplayConfigurationObserverMultiplexer(std::shared_ptr<Executor> const& default_executor);

    void register_interest(std::weak_ptr<DisplayConfigurationObserver> const& observer, Executor& executor) override;

    void register_early_observer(
        std::weak_ptr<DisplayConfigurationObserver> const& observer,
        Executor& executor) override;

    void initial_configuration(std::shared_ptr<DisplayConfiguration const> const& config) override;

    void configuration_applied(std::shared_ptr<DisplayConfiguration const> const& config) override;

    void base_configuration_updated(std::shared_ptr<DisplayConfiguration const> const& base_config) override;

    void session_configuration_applied(std::shared_ptr<scene::Session> const& session,
                                       std::shared_ptr<DisplayConfiguration> const& config) override;

    void session_configuration_removed(std::shared_ptr<scene::Session> const& session) override;

    void configuration_failed(
        std::shared_ptr<DisplayConfiguration const> const& attempted,
        std::exception const& error) override;

    void catastrophic_configuration_error(
        std::shared_ptr<DisplayConfiguration const> const& failed_fallback,
        std::exception const& error) override;

    void configuration_updated_for_session(
        std::shared_ptr<scene::Session> const& session,
        std::shared_ptr<DisplayConfiguration const> const& config) override;

private:
    std::shared_ptr<Executor> const executor;

    std::mutex mutex;
    std::shared_ptr<DisplayConfiguration const> current_config;
};

}
}

#endif //MIR_DISPLAY_CONFIGURATION_MULTIPLEXER_H_
