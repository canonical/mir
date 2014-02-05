/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "null_report_factory.h"

#include "mir/compositor/compositor_report.h"
#include "mir/frontend/connector_report.h"
#include "mir/frontend/null_message_processor_report.h"
#include "mir/frontend/session_mediator_report.h"
#include "mir/graphics/null_display_report.h"
#include "mir/input/null_input_report.h"
#include "mir/scene/scene_report.h"

std::shared_ptr<mir::compositor::CompositorReport> mir::NullReportFactory::create_compositor_report()
{
    return std::make_shared<compositor::NullCompositorReport>();
}

std::shared_ptr<mir::graphics::DisplayReport> mir::NullReportFactory::create_display_report()
{
    return std::make_shared<graphics::NullDisplayReport>();
}

std::shared_ptr<mir::scene::SceneReport> mir::NullReportFactory::create_scene_report()
{
    return std::make_shared<scene::NullSceneReport>();
}

std::shared_ptr<mir::frontend::ConnectorReport> mir::NullReportFactory::create_connector_report()
{
    return std::make_shared<frontend::NullConnectorReport>();
}

std::shared_ptr<mir::frontend::SessionMediatorReport> mir::NullReportFactory::create_session_mediator_report()
{
    return std::make_shared<frontend::NullSessionMediatorReport>();
}

std::shared_ptr<mir::frontend::MessageProcessorReport> mir::NullReportFactory::create_message_processor_report()
{
    return std::make_shared<frontend::NullMessageProcessorReport>();
}

std::shared_ptr<mir::input::InputReport> mir::NullReportFactory::create_input_report()
{
    return std::make_shared<input::NullInputReport>();
}



