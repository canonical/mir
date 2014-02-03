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
#include "create_report.h"
#include "../lttng/display_report.h"
#include "../lttng/compositor_report.h"
#include "../lttng/connector_report.h"
#include "../lttng/session_mediator_report.h"
#include "session_mediator_report.h"

#include "mir/graphics/null_display_report.h"
#include "compositor_report.h"

using namespace mir;
namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace mc = mir::compositor;
namespace ml = mir::logging;
namespace ms = mir::scene;

auto mir::DefaultServerConfiguration::the_connector_report()
    -> std::shared_ptr<mf::ConnectorReport>
{
    return connector_report(std::bind(&ml::create_report<mf::ConnectorReport,
                                                         ml::ConnectorReport,
                                                         mir::lttng::ConnectorReport,
                                                         mf::NullConnectorReport>,
                                      this,
                                      the_options(),
                                      connector_report_opt));
}

std::shared_ptr<mg::DisplayReport> mir::DefaultServerConfiguration::the_display_report()
{
    return display_report(std::bind(
        &ml::create_report<mg::DisplayReport, ml::DisplayReport, mir::lttng::DisplayReport, mg::NullDisplayReport>,
        this,
        the_options(),
        display_report_opt));
}

std::shared_ptr<compositor::CompositorReport>
DefaultServerConfiguration::the_compositor_report()
{
    return compositor_report(std::bind(&ml::create_report_with_clock<mc::CompositorReport,
                                                                     ml::CompositorReport,
                                                                     mir::lttng::CompositorReport,
                                                                     mc::NullCompositorReport>,
                                       this,
                                       the_options(),
                                       compositor_report_opt));
}

std::shared_ptr<mf::SessionMediatorReport>
mir::DefaultServerConfiguration::the_session_mediator_report()
{
    return session_mediator_report(std::bind(&ml::create_report<mf::SessionMediatorReport,
                                                                ml::SessionMediatorReport,
                                                                mir::lttng::SessionMediatorReport,
                                                                mf::NullSessionMediatorReport>,
                                             this,
                                             the_options(),
                                             session_mediator_report_opt));
}
