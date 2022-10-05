/*
 * Copyright © 2022 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
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

#ifndef MIRAL_FD_HANDLE_H
#define MIRAL_FD_HANDLE_H

#include <memory>

namespace miral
{
/// A handle returned by FdManager::register_handler() which keeps a file descriptor open and
/// watched in the main loop until it is no longer held.
struct FdHandle { public: virtual ~FdHandle() {} };
}

#endif //MIRAL_FD_HANDLE_H