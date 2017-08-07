/*
 * Copyright © 2016 Canonical Ltd.
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

#ifndef MIR_COOKIE_COOKIE_FORMAT_H_
#define MIR_COOKIE_COOKIE_FORMAT_H_

#include <stdint.h>

namespace mir
{
namespace cookie
{
enum class Format : uint8_t
{
    hmac_sha_256,
};
}
}

#endif // MIR_COOKIE_COOKIE_FORMAT_H_
