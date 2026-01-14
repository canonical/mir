/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_ERRNO_UTILS_H_
#define MIR_ERRNO_UTILS_H_

namespace mir
{
// Thread-safe errno to char* conversion. Returns a buffer that remains valid
// until the next call to this function in the same thread.
auto errno_to_cstr(int err) noexcept -> const char*;

} // namespace mir

#endif /* MIR_ERRNO_UTILS_H_ */
