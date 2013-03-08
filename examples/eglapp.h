/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
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
 * Author: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#ifndef __EGLAPP_H__
#define __EGLAPP_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef int mir_eglapp_bool;

mir_eglapp_bool mir_eglapp_init(int *width, int *height);
void            mir_eglapp_swap_buffers(void);
mir_eglapp_bool mir_eglapp_running(void);
void            mir_eglapp_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif
