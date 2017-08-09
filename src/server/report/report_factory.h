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

#ifndef MIR_REPORT_REPORT_FACTORY_H_
#define MIR_REPORT_REPORT_FACTORY_H_

#include <memory>

namespace mir
{
class SharedLibraryProberReport;
namespace compositor
{
class CompositorReport;
}
namespace frontend
{
class ConnectorReport;
class SessionMediatorObserver;
class MessageProcessorReport;
}
namespace graphics
{
class DisplayReport;
}
namespace input
{
class InputReport;
class SeatObserver;
}
namespace scene
{
class SceneReport;
}
namespace shell { class ShellReport; }

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
    virtual std::shared_ptr<frontend::SessionMediatorObserver> create_session_mediator_report() = 0;
    virtual std::shared_ptr<frontend::MessageProcessorReport> create_message_processor_report() = 0;
    virtual std::shared_ptr<input::InputReport> create_input_report() = 0;
    virtual std::shared_ptr<input::SeatObserver> create_seat_report() = 0;
    virtual std::shared_ptr<SharedLibraryProberReport> create_shared_library_prober_report() = 0;
    virtual std::shared_ptr<shell::ShellReport> create_shell_report() = 0;

protected:
    ReportFactory() = default;
    ReportFactory(ReportFactory const&) = delete;
    ReportFactory& operator=(ReportFactory const &) = delete;
};

}
}

#endif /* MIR_REPORT_REPORT_FACTORY_H_ */
