/*
 * Copyright © 2016 Canonical Ltd.
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
 */

#ifndef MIR_TOOLKIT_CLIENT_TYPES_NBS_H_
#define MIR_TOOLKIT_CLIENT_TYPES_NBS_H_

#include <mir_toolkit/client_types.h>

#ifdef __cplusplus
/**
 * \defgroup mir_toolkit MIR graphics tools API
 * @{
 */
extern "C" {
#endif

/* NOTE: this file will be rolled into mir_toolkit/client_types.h when made public. */
typedef struct MirPresentationChain MirPresentationChain;
typedef struct MirBuffer MirBuffer;
typedef void* MirNativeFence;

typedef void (*mir_buffer_callback)(MirPresentationChain*, MirBuffer*, void* context);
typedef void (*mir_presentation_chain_callback)(MirPresentationChain*, void* context);
typedef enum MirBufferAccess
{
    mir_none,
    mir_read,
    mir_read_write,
} MirBufferAccess;

#ifdef __cplusplus
}
/**@}*/
#endif

#endif /* MIR_TOOLKIT_CLIENT_TYPES_NBS_H_ */
