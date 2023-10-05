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

#ifndef MIR_MAKE_SHM_POOL_H
#define MIR_MAKE_SHM_POOL_H

#include <wayland-client.h>

#ifdef __cplusplus
extern "C" {
#endif

struct wl_shm_pool*
make_shm_pool(struct wl_shm* shm, int size, void** data);

#ifdef __cplusplus
}
#endif

#endif //MIR_MAKE_SHM_POOL_H
