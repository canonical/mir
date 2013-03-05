/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */


#ifndef MIR_ANDROID_UBUNTU_ERRORS_H_
#define MIR_ANDROID_UBUNTU_ERRORS_H_

#include <sys/types.h>
#include <errno.h>

namespace mir_input
{

// use this type to return error codes
#ifdef HAVE_MS_C_RUNTIME
typedef int         status_t;
#else
typedef int32_t     status_t;
#endif

/* the MS C runtime lacks a few error codes */

/*
 * Error codes.
 * All error codes are negative values.
 */

// Win32 #defines NO_ERROR as well.  It has the same value, so there's no
// real conflict, though it's a bit awkward.
#ifdef _WIN32
# undef NO_ERROR
#endif

enum {
    OK                = 0,    // Everything's swell.
    NO_ERROR          = 0,    // No errors.

    UNKNOWN_ERROR       = 0x80000000,

    NO_MEMORY           = -ENOMEM,
    INVALID_OPERATION   = -ENOSYS,
    BAD_VALUE           = -EINVAL,
    BAD_TYPE            = 0x80000001,
    NAME_NOT_FOUND      = -ENOENT,
    PERMISSION_DENIED   = -EPERM,
    NO_INIT             = -ENODEV,
    ALREADY_EXISTS      = -EEXIST,
    DEAD_OBJECT         = -EPIPE,
    FAILED_TRANSACTION  = 0x80000002,
    JPARKS_BROKE_IT     = -EPIPE,
#if !defined(HAVE_MS_C_RUNTIME)
    BAD_INDEX           = -EOVERFLOW,
    NOT_ENOUGH_DATA     = -ENODATA,
    WOULD_BLOCK         = -EWOULDBLOCK,
    TIMED_OUT           = -ETIMEDOUT,
    UNKNOWN_TRANSACTION = -EBADMSG,
#else
    BAD_INDEX           = -E2BIG,
    NOT_ENOUGH_DATA     = 0x80000003,
    WOULD_BLOCK         = 0x80000004,
    TIMED_OUT           = 0x80000005,
    UNKNOWN_TRANSACTION = 0x80000006,
#endif
    FDS_NOT_ALLOWED     = 0x80000007
};

// Restore define; enumeration is in "android" namespace, so the value defined
// there won't work for Win32 code in a different namespace.
#ifdef _WIN32
# define NO_ERROR 0L
#endif

}

namespace android
{
using ::mir_input::status_t;
using ::mir_input::OK;
using ::mir_input::NO_ERROR;
using ::mir_input::UNKNOWN_ERROR;
using ::mir_input::NO_MEMORY;
using ::mir_input::INVALID_OPERATION;
using ::mir_input::BAD_VALUE;
using ::mir_input::BAD_TYPE;
using ::mir_input::NAME_NOT_FOUND;
using ::mir_input::PERMISSION_DENIED;
using ::mir_input::NO_INIT;
using ::mir_input::ALREADY_EXISTS;
using ::mir_input::DEAD_OBJECT;
using ::mir_input::FAILED_TRANSACTION;
using ::mir_input::JPARKS_BROKE_IT;
using ::mir_input::BAD_INDEX;
using ::mir_input::NOT_ENOUGH_DATA;
using ::mir_input::WOULD_BLOCK;
using ::mir_input::TIMED_OUT;
using ::mir_input::UNKNOWN_TRANSACTION;
using ::mir_input::FDS_NOT_ALLOWED;
}


#endif /* MIR_ANDROID_UBUNTU_ERRORS_H_ */
