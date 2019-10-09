/*
 * Stub client to be compiled with -std=c99, just to check we have correct
 * language compatibility in the client header(s).
 *
 * Copyright © 2013-2019 Canonical Ltd.
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
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 *
 */

#include "mir_toolkit/mir_client_library.h"
#include "mir_toolkit/event.h"
#include "mir_toolkit/extensions/fenced_buffers.h"
#include "mir_toolkit/extensions/gbm_buffer.h"
#include "mir_toolkit/extensions/graphics_module.h"
#include "mir_toolkit/extensions/mesa_drm_auth.h"
#include "mir_toolkit/extensions/set_gbm_device.h"
#include "mir_toolkit/extensions/window_coordinate_translation.h"

int main()
{
    return 0;
}
