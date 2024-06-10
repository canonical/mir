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

#include "../null_report_factory.h"

#include "compositor_report.h"
#include "display_report.h"
#include "input_report.h"
#include "seat_report.h"
#include "mir/report/null/shell_report.h"
#include "scene_report.h"
#include "mir/logging/null_shared_library_prober_report.h"

std::shared_ptr<mir::compositor::CompositorReport> mir::report::NullReportFactory::create_compositor_report()
{
    return std::make_shared<null::CompositorReport>();
}

std::shared_ptr<mir::graphics::DisplayReport> mir::report::NullReportFactory::create_display_report()
{
    return std::make_shared<null::DisplayReport>();
}

std::shared_ptr<mir::scene::SceneReport> mir::report::NullReportFactory::create_scene_report()
{
    return std::make_shared<null::SceneReport>();
}

std::shared_ptr<mir::input::InputReport> mir::report::NullReportFactory::create_input_report()
{
    return std::make_shared<null::InputReport>();
}

std::shared_ptr<mir::input::SeatObserver> mir::report::NullReportFactory::create_seat_report()
{
    return std::make_shared<null::SeatReport>();
}

std::shared_ptr<mir::SharedLibraryProberReport> mir::report::NullReportFactory::create_shared_library_prober_report()
{
    return std::make_shared<logging::NullSharedLibraryProberReport>();
}

std::shared_ptr<mir::shell::ShellReport> mir::report::NullReportFactory::create_shell_report()
{
    return std::make_shared<null::ShellReport>();
}

std::shared_ptr<mir::compositor::CompositorReport> mir::report::null_compositor_report()
{
    return NullReportFactory{}.create_compositor_report();
}

std::shared_ptr<mir::SharedLibraryProberReport> mir::report::null_shared_library_prober_report()
{
    return NullReportFactory{}.create_shared_library_prober_report();
}

std::shared_ptr<mir::graphics::DisplayReport> mir::report::null_display_report()
{
    return NullReportFactory{}.create_display_report();
}
std::shared_ptr<mir::scene::SceneReport> mir::report::null_scene_report()
{
    return NullReportFactory{}.create_scene_report();
}
std::shared_ptr<mir::input::InputReport> mir::report::null_input_report()
{
    return NullReportFactory{}.create_input_report();
}

std::shared_ptr<mir::input::SeatObserver> mir::report::null_seat_report()
{
    return NullReportFactory{}.create_seat_report();
}
