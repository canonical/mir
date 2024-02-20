/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_GRAPHICS_PLATFORM_PROBE_H_
#define MIR_GRAPHICS_PLATFORM_PROBE_H_

#include <vector>
#include <memory>
#include "mir/shared_library.h"
#include "mir/options/program_option.h"
#include "mir/graphics/platform.h"
#include "mir/shared_library_prober_report.h"

namespace mir
{
class ConsoleServices;

namespace graphics
{
enum class TypePreference
{
    prefer_nested,
    prefer_hardware
};

auto modules_for_device(
    std::function<std::vector<SupportedDevice>(mir::SharedLibrary const&)> const& probe,
    std::vector<std::shared_ptr<SharedLibrary>> const& modules,
    TypePreference nested_selection)
    -> std::vector<std::pair<SupportedDevice, std::shared_ptr<mir::SharedLibrary>>>;

auto probe_display_module(
    SharedLibrary const& module,
    options::ProgramOption const& options,
    std::shared_ptr<ConsoleServices> const& console) -> std::vector<SupportedDevice>;

auto probe_rendering_module(
    std::span<std::shared_ptr<DisplayPlatform>> const& platforms,
    SharedLibrary const& module,
    options::ProgramOption const& options,
    std::shared_ptr<ConsoleServices> const& console) -> std::vector<SupportedDevice>;

auto display_modules_for_device(
    std::vector<std::shared_ptr<SharedLibrary>> const& modules,
    options::ProgramOption const& options,
    std::shared_ptr<ConsoleServices> const& console)
    -> std::vector<std::pair<SupportedDevice, std::shared_ptr<SharedLibrary>>>;

auto rendering_modules_for_device(
    std::vector<std::shared_ptr<SharedLibrary>> const& modules,
    std::span<std::shared_ptr<DisplayPlatform>> const& platforms,
    options::ProgramOption const& options,
    std::shared_ptr<ConsoleServices> const& console)
    -> std::vector<std::pair<SupportedDevice, std::shared_ptr<SharedLibrary>>>;

auto select_display_modules(
    options::Option const& options,
    std::shared_ptr<ConsoleServices> const& console,
    SharedLibraryProberReport& lib_loader_report)
    -> std::vector<std::pair<SupportedDevice, std::shared_ptr<SharedLibrary>>>;
}
}

#endif // MIR_GRAPHICS_PLATFORM_PROBE_H_
