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
 *
 */

#ifndef MIR_TOOLKIT_MIR_PLATFORM_MESSAGE_H_
#define MIR_TOOLKIT_MIR_PLATFORM_MESSAGE_H_

#include <sys/types.h>

#ifdef __cplusplus
/**
 * \addtogroup mir_toolkit
 * @{
 */
extern "C" {
#endif

struct MirPlatformMessage;

typedef struct
{
    void const* const data;
    size_t const size;
} MirPlatformMessageData;

typedef struct
{
    int const* const fds;
    size_t const num_fds;
} MirPlatformMessageFds;

/**
 * Create a platform message to use with mir_connection_platform_operation().
 *
 * Each call to mir_platform_message_create() should be matched by
 * a call to mir_platform_message_release() to avoid memory leaks.
 *
 *   \param [in] opcode    The platform message opcode
 *   \return               The created MirPlatformMessage
 */
MirPlatformMessage* mir_platform_message_create(unsigned int opcode);

/**
 * Release a platform message.
 *
 *   \param [in] message   The MirPlatformMessage
 */
void mir_platform_message_release(MirPlatformMessage const* message);

/**
 * Set the data associated with a message.
 *
 * The data is copied into the message.
 *
 *   \param [in] message    The MirPlatformMessage
 *   \param [in] data       Pointer to the data
 *   \param [in] data_size  The size of the data in bytes
 */
void mir_platform_message_set_data(MirPlatformMessage* message, void const* data, size_t data_size);

/**
 * Sets the fds associated with a message.
 *
 * The fd array is copied into the message, but the message does not take
 * ownership of the fds, i.e., the caller is responsible for keeping
 * the fds open for as long as this message needs to remain valid.
 *
 * Note that the fds associated with a message are not closed when the message
 * is released. The caller is responsible for closing the fds when the message
 * doesn't need them anymore (see also mir_platform_message_get_fds()).
 *
 *   \param [in] message   The MirPlatformMessage
 *   \param [in] fds       Pointer to the array of fds
 *   \param [in] num_fds   The number of fds
 */
void mir_platform_message_set_fds(MirPlatformMessage* message, int const* fds, size_t num_fds);

/**
 * Get the opcode of a message.
 *
 *   \param [in] message   The MirPlatformMessage
 *   \return               The opcode
 */
unsigned int mir_platform_message_get_opcode(MirPlatformMessage const* message);

/**
 * Get the data associated with a message.
 *
 * The memory holding the returned data array is owned by the message and is
 * valid only as long as the message is valid and mir_platform_set_data() is
 * not called. You must not change or free the returned data array.
 *
 *   \param [in] message   The MirPlatformMessage
 *   \return               The data
 */
MirPlatformMessageData mir_platform_message_get_data(MirPlatformMessage const* message);

/**
 * Gets the fds associated with a message.
 *
 * The memory of the returned fd array is owned by the message and is valid
 * only as long as the message is valid and mir_platform_set_fds() is not
 * called. You must not change or free the returned fd array.
 *
 * Note that the fds associated with a message will not be closed when the
 * message is released. Users are responsible for getting and closing the
 * fds to avoid leaks.
 *
 *   \param [in] message   The MirPlatformMessage
 *   \return               The fds
 */
MirPlatformMessageFds mir_platform_message_get_fds(MirPlatformMessage const* message);

#ifdef __cplusplus
}
/**@}*/
#endif

#endif
