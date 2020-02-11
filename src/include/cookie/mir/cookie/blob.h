/*
 * Copyright © 2015-2016 Canonical Ltd.
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
 *
 * Authored by: Brandon Schaefer <brandon.schaefer@canonical.com>
 */

#ifndef MIR_COOKIE_BLOB_H_
#define MIR_COOKIE_BLOB_H_

#include <stddef.h>
#include <stdint.h>
#include <array>

namespace mir
{
namespace cookie
{
size_t const default_blob_size = 41;
using Blob = std::array<uint8_t, default_blob_size>;
}
}

#endif // MIR_COOKIE_BLOB_H_
