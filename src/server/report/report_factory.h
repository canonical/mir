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

#ifndef MIR_REPORT_REPORT_FACTORY_H_
#define MIR_REPORT_REPORT_FACTORY_H_

#include <memory>

namespace mir
{
namespace compositor
{
class CompositorReport;
}
namespace frontend
{
class ConnectorReport;
class SessionMediatorReport;
class MessageProcessorReport;
}
namespace graphics
{
class DisplayReport;
}
namespace input
{
class InputReport;
}
namespace scene
{
class SceneReport;
}

namespace report
{
class ReportFactory
{
public:
    virtual ~ReportFactory() = default;
    virtual std::shared_ptr<compositor::CompositorReport> create_compositor_report() = 0;
    virtual std::shared_ptr<graphics::DisplayReport> create_display_report() = 0;
    virtual std::shared_ptr<scene::SceneReport> create_scene_report() = 0;
    virtual std::shared_ptr<frontend::ConnectorReport> create_connector_report() = 0;
    virtual std::shared_ptr<frontend::SessionMediatorReport> create_session_mediator_report() = 0;
    virtual std::shared_ptr<frontend::MessageProcessorReport> create_message_processor_report() = 0;
    virtual std::shared_ptr<input::InputReport> create_input_report() = 0;

protected:
    ReportFactory() = default;
    ReportFactory(ReportFactory const&) = delete;
    ReportFactory& operator=(ReportFactory const &) = delete;
};

}
}

#endif /* MIR_REPORT_REPORT_FACTORY_H_ */
