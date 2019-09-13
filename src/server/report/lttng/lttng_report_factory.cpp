/*
 * Copyright © 2014 Canonical Ltd.
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
 *
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "../lttng_report_factory.h"

#include "compositor_report.h"
#include "connector_report.h"
#include "display_report.h"
#include "input_report.h"
#include "message_processor_report.h"
#include "scene_report.h"
#include "session_mediator_report.h"
#include "shared_library_prober_report.h"
#include <boost/throw_exception.hpp>

std::shared_ptr<mir::compositor::CompositorReport> mir::report::LttngReportFactory::create_compositor_report()
{
    return std::make_shared<lttng::CompositorReport>();
}

std::shared_ptr<mir::graphics::DisplayReport> mir::report::LttngReportFactory::create_display_report()
{
    return std::make_shared<lttng::DisplayReport>();
}

std::shared_ptr<mir::scene::SceneReport> mir::report::LttngReportFactory::create_scene_report()
{
    return std::make_shared<lttng::SceneReport>();
}

std::shared_ptr<mir::frontend::ConnectorReport> mir::report::LttngReportFactory::create_connector_report()
{
    return std::make_shared<lttng::ConnectorReport>();
}

std::shared_ptr<mir::frontend::SessionMediatorObserver> mir::report::LttngReportFactory::create_session_mediator_report()
{
    return std::make_shared<lttng::SessionMediatorReport>();
}

std::shared_ptr<mir::frontend::MessageProcessorReport> mir::report::LttngReportFactory::create_message_processor_report()
{
    return std::make_shared<lttng::MessageProcessorReport>();
}

std::shared_ptr<mir::input::InputReport> mir::report::LttngReportFactory::create_input_report()
{
    return std::make_shared<lttng::InputReport>();
}

std::shared_ptr<mir::input::SeatObserver> mir::report::LttngReportFactory::create_seat_report()
{
    BOOST_THROW_EXCEPTION(std::logic_error("Not implemented"));
}

std::shared_ptr<mir::SharedLibraryProberReport> mir::report::LttngReportFactory::create_shared_library_prober_report()
{
    return std::make_shared<lttng::SharedLibraryProberReport>();
}

std::shared_ptr<mir::shell::ShellReport> mir::report::LttngReportFactory::create_shell_report()
{
    BOOST_THROW_EXCEPTION(std::logic_error("Not implemented"));
}
