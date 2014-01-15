/*
 * Copyright © 2013 Canonical Ltd.
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

#include "mir/default_server_configuration.h"
#include "mir/abnormal_exit.h"

#include "connector_report.h"
#include "display_report.h"
#include "session_mediator_report.h"

#include "mir/graphics/null_display_report.h"
#include "compositor_report.h"

using namespace mir;
namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace ml = mir::logging;
namespace ms = mir::scene;

auto mir::DefaultServerConfiguration::the_connector_report()
    -> std::shared_ptr<mf::ConnectorReport>
{
    return connector_report([this]
        () -> std::shared_ptr<mf::ConnectorReport>
        {
            auto opt = the_options()->get(connector_report_opt, off_opt_value);

            if (opt == log_opt_value)
            {
                return std::make_shared<ml::ConnectorReport>(the_logger());
            }
            else if (opt == off_opt_value)
            {
                return std::make_shared<mf::NullConnectorReport>();
            }
            else
            {
                throw AbnormalExit(std::string("Invalid ") + connector_report_opt + " option: " + opt +
                    " (valid options are: \"" + off_opt_value + "\" and \"" + log_opt_value + "\")");
            }
        });
}

std::shared_ptr<mg::DisplayReport> mir::DefaultServerConfiguration::the_display_report()
{
    return display_report(
        [this]() -> std::shared_ptr<graphics::DisplayReport>
        {
            if (the_options()->get(display_report_opt, off_opt_value) == log_opt_value)
            {
                return std::make_shared<ml::DisplayReport>(the_logger());
            }
            else
            {
                return std::make_shared<mg::NullDisplayReport>();
            }
        });
}

std::shared_ptr<compositor::CompositorReport>
DefaultServerConfiguration::the_compositor_report()
{
    return compositor_report(
        [this]() -> std::shared_ptr<compositor::CompositorReport>
        {
            if (the_options()->get(compositor_report_opt, off_opt_value) == log_opt_value)
            {
                return std::make_shared<ml::CompositorReport>(
                    the_logger(), the_clock());
            }
            else
            {
                return std::make_shared<compositor::NullCompositorReport>();
            }
        });
}

std::shared_ptr<mf::SessionMediatorReport>
mir::DefaultServerConfiguration::the_session_mediator_report()
{
    return session_mediator_report(
        [this]() -> std::shared_ptr<mf::SessionMediatorReport>
        {
            if (the_options()->get(session_mediator_report_opt, off_opt_value) == log_opt_value)
            {
                return std::make_shared<ml::SessionMediatorReport>(the_logger());
            }
            else
            {
                return std::make_shared<mf::NullSessionMediatorReport>();
            }
        });
}
