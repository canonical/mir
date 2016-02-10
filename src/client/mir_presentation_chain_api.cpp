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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir_toolkit/mir_presentation_chain.h"
#include "mir_toolkit/mir_buffer.h"
#include "mir_connection.h"
#include "buffer.h"
#include "presentation_chain.h"
#include "mir/uncaught.h"
#include <stdexcept>
#include <boost/throw_exception.hpp>
namespace mcl = mir::client;

namespace
{
// assign_result is compatible with all 2-parameter callbacks
void assign_result(void* result, void** context)
{
    if (context)
        *context = result;
}
}
//private NBS api under development
bool mir_presentation_chain_submit_buffer(MirPresentationChain* client_chain, MirBuffer* buffer)
try
{
    auto chain = reinterpret_cast<mcl::MirPresentationChain*>(client_chain);
    chain->submit_buffer(buffer);
    return true;
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    return false;
}

bool mir_presentation_chain_is_valid(MirPresentationChain* chain)
{
    return mir_presentation_chain_get_error_message(chain) == std::string("");
}

char const *mir_presentation_chain_get_error_message(MirPresentationChain* client_chain)
{
    auto chain = reinterpret_cast<mcl::MirPresentationChain*>(client_chain);
    return chain->error_msg().c_str();
}

MirWaitHandle* mir_connection_create_presentation_chain(
    MirConnection* connection, mir_presentation_chain_callback callback, void* context)
try
{
    return connection->create_presentation_chain(callback, context);
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    return nullptr;
}

MirPresentationChain* mir_connection_create_presentation_chain_sync(MirConnection* connection)
{
    MirPresentationChain *context = nullptr;
    mir_connection_create_presentation_chain(connection,
        reinterpret_cast<mir_presentation_chain_callback>(assign_result), &context)->wait_for_all();
    return context;
}

void mir_presentation_chain_release(MirPresentationChain* client_chain)
try
{
    auto chain = reinterpret_cast<mcl::MirPresentationChain*>(client_chain);
    chain->connection()->release_presentation_chain(client_chain);
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}
