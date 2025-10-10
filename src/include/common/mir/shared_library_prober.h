/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_SHARED_LIBRARY_PROBER_H_
#define MIR_SHARED_LIBRARY_PROBER_H_

#include "shared_library_prober_report.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace mir
{
class SharedLibrary;

std::vector<std::shared_ptr<SharedLibrary>> libraries_for_path(std::string const& path, SharedLibraryProberReport& report);

// The selector can tell select_libraries_for_path() to persist or quit after each library
enum class Selection { persist, quit };

void select_libraries_for_path(
    std::string const& path,
    std::function<Selection(std::shared_ptr<SharedLibrary> const&)> const& selector,
    SharedLibraryProberReport& report);
}


#endif /* MIR_SHARED_LIBRARY_PROBER_H_ */
