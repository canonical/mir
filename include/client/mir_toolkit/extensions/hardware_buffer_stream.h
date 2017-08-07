/*
 * Copyright © 2017 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2 or 3
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_CLIENT_EXTENSIONS_HARDWARE_BUFFER_STREAM_H_
#define MIR_CLIENT_EXTENSIONS_HARDWARE_BUFFER_STREAM_H_

#include "mir_toolkit/rs/mir_render_surface.h"
#include "mir_toolkit/mir_buffer_stream.h"
#include "mir_toolkit/mir_extension_core.h"

#ifdef __cplusplus
extern "C" {
#endif

//This extension should probably not be published.
//Mesa's egl will use MirPresentationChain + MirBuffer, so there's no need
//for a 'hardware' buffer stream publicly.
//Internally, its useful in playground/ and the nested server until we switch upstream
//driver stacks to the mir egl platform. 

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
typedef MirBufferStream* (*MirExtensionGetHardwareBufferStream)(
    MirRenderSurface* rs,
    int width, int height,
    MirPixelFormat format);
#pragma GCC diagnostic pop

typedef struct MirExtensionHardwareBufferStreamV1
{
    MirExtensionGetHardwareBufferStream get_hardware_buffer_stream;
} MirExtensionHardwareBufferStreamV1;

static inline MirExtensionHardwareBufferStreamV1 const* mir_extension_hardware_buffer_stream_v1(
    MirConnection* connection)
{
    return (MirExtensionHardwareBufferStreamV1 const*) mir_connection_request_extension(
        connection, "mir_extension_hardware_buffer_stream", 1);
}

#ifdef __cplusplus
}
#endif
#endif /* MIR_CLIENT_EXTENSIONS_HARDWARE_BUFFER_STREAM_H_ */
