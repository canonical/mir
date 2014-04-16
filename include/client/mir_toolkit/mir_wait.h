/*
 * Copyright © 2012-2014 Canonical Ltd.
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

#ifndef MIR_TOOLKIT_MIR_WAIT_H_
#define MIR_TOOLKIT_MIR_WAIT_H_

#include <mir_toolkit/client_types.h>

#ifdef __cplusplus
/**
 * \addtogroup mir_toolkit
 * @{
 */
extern "C" {
#endif

struct MirWaitHandle;

/**
 * Wait on the supplied handle until all instances of the associated request
 * have completed.
 *   \param [in] wait_handle  Handle returned by an asynchronous request
 */
void mir_wait_for(MirWaitHandle *wait_handle);

/**
 * Wait on the supplied handle until one instance of the associated request
 * has completed. Use this instead of mir_wait_for in a threaded environment
 * to ensure that the act of waiting does not clear all results associated
 * with the wait handle; only one.
 *   \param [in] wait_handle  Handle returned by an asynchronous request
 */
void mir_wait_for_one(MirWaitHandle *wait_handle);


#ifdef __cplusplus
}
/**@}*/
#endif

#endif /* MIR_TOOLKIT_MIR_WAIT_H_ */
