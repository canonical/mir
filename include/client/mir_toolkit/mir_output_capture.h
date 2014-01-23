/*
 * Copyright © 2014 Canonical Ltd.
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
 */

#ifndef MIR_TOOLKIT_MIR_OUTPUT_CAPTURE_H_
#define MIR_TOOLKIT_MIR_OUTPUT_CAPTURE_H_

#include <mir_toolkit/client_types.h>

#ifdef __cplusplus
/**
 * \addtogroup mir_toolkit
 * @{
 */
extern "C" {
#endif

MirOutputCapture *mir_connection_create_output_capture_sync(
    MirConnection *connection,
    MirOutputCaptureParameters *parameters);

void mir_output_capture_release_sync(
    MirOutputCapture *capture);

MirEGLNativeWindowType mir_output_capture_egl_native_window(
    MirOutputCapture *capture);

#ifdef __cplusplus
}
/**@}*/
#endif

#endif /* MIR_TOOLKIT_MIR_OUTPUT_CAPTURE_H_ */
