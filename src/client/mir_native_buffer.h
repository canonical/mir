/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *   Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_CLIENT_MIR_NATIVE_BUFFER_H_
#define MIR_CLIENT_MIR_NATIVE_BUFFER_H_

#ifdef ANDROID
struct ANativeWindowBuffer;
typedef ANativeWindowBuffer* MirNativeBuffer;
#else
typedef void* MirNativeBuffer;
#endif

#endif /* MIR_CLIENT_MIR_NATIVE_BUFFER_H_ */
