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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/test/doubles/platform_factory.h"

#include "mir/graphics/platform.h"

#ifdef MESA_KMS
#include "src/platforms/mesa/server/kms/platform.h"
#elif MESA_X11
#include "src/platforms/mesa/server/x11/graphics/platform.h"
#endif

#include "src/server/report/null_report_factory.h"
#include "mir/test/doubles/null_virtual_terminal.h"
#include "mir/test/doubles/null_emergency_cleanup.h"
#include "mir/options/program_option.h"

namespace mtd = mir::test::doubles;

#ifdef ANDROID
auto mtd::create_platform_with_null_dependencies()
    -> std::shared_ptr<graphics::Platform>
{
    return create_host_platform(
        std::make_shared<options::ProgramOption>(),
        std::make_shared<NullEmergencyCleanup>(),
        report::null_display_report());
}

#elif MESA_KMS
auto mtd::create_platform_with_null_dependencies()
    -> std::shared_ptr<graphics::Platform>
{
    return create_mesa_platform_with_null_dependencies();
}

auto mtd::create_mesa_platform_with_null_dependencies()
    -> std::shared_ptr<graphics::mesa::Platform>
{
    return std::make_shared<graphics::mesa::Platform>(
        report::null_display_report(),
        std::make_shared<NullVirtualTerminal>(),
        *std::make_shared<NullEmergencyCleanup>(),
        graphics::mesa::BypassOption::allowed);
}
#elif MESA_X11
auto mtd::create_platform_with_null_dependencies()
    -> std::shared_ptr<graphics::Platform>
{
    return std::make_shared<graphics::X::Platform>(std::shared_ptr<::Display>(
                                                       XOpenDisplay(nullptr),
                                                       [](::Display* display)
                                                       {
                                                           XCloseDisplay(display);
                                                       }));
}
#endif
