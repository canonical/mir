/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
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
#include "mir_toolkit/mir_buffer_private.h"
#include "mir_connection.h"
#include "connection_surface_map.h"
#include "buffer.h"
#include "mir_presentation_chain.h"
#include "mir/uncaught.h"
#include "mir/require.h"
#include <stdexcept>
#include <boost/throw_exception.hpp>
namespace mcl = mir::client;

//private NBS api under development
void mir_presentation_chain_submit_buffer(
    MirPresentationChain* chain,
    MirBuffer* b,
    MirBufferCallback available_callback, void* available_context)
try
{
    auto buffer = reinterpret_cast<mcl::MirBuffer*>(b);
    mir::require(chain && buffer && mir_presentation_chain_is_valid(chain));
    buffer->set_callback(available_callback, available_context);
    chain->submit_buffer(buffer);
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

bool mir_presentation_chain_is_valid(MirPresentationChain* chain)
try
{
    mir::require(chain);
    return mir_presentation_chain_get_error_message(chain) == std::string("");
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    return false;
}

char const *mir_presentation_chain_get_error_message(MirPresentationChain* chain)
try
{
    mir::require(chain);
    return chain->error_msg();
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    return "error accessing error message";
}

void mir_presentation_chain_set_queueing_mode(MirPresentationChain* chain)
try
{
    mir::require(chain);
    chain->set_queueing_mode();
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

void mir_presentation_chain_set_dropping_mode(MirPresentationChain* chain)
try
{
    mir::require(chain);
    chain->set_dropping_mode();
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

bool mir_connection_present_mode_supported(
    MirConnection*, MirPresentMode mode)
{
    return mode == mir_present_mode_fifo || mode == mir_present_mode_mailbox;
}

void mir_presentation_chain_set_mode(
    MirPresentationChain* chain, MirPresentMode mode)
try
{
    mir::require(chain && mir_connection_present_mode_supported(chain->connection(), mode));
    if (mode == mir_present_mode_fifo)
        chain->set_queueing_mode();
    if (mode == mir_present_mode_mailbox)
        chain->set_dropping_mode();
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}
