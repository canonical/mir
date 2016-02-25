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
#include "mir_presentation_chain.h"
#include "mir/uncaught.h"
#include "mir/require.h"
#include <stdexcept>
#include <boost/throw_exception.hpp>
namespace mcl = mir::client;

namespace
{
class ChainResult
{
public:
    void set_result(MirPresentationChain* result)
    {
        std::unique_lock<decltype(mutex)> lk(mutex);
        chain = result;
        cv.notify_all();
    }

    MirPresentationChain* wait_for_result()
    {
        std::unique_lock<decltype(mutex)> lk(mutex);
        cv.wait(lk, [this] { return chain; });
        return chain;
    }

private:
    MirPresentationChain* chain = nullptr;
    std::mutex mutex;
    std::condition_variable cv;
};

void set_result(MirPresentationChain* result, ChainResult* context)
{
    context->set_result(result);
}
}
//private NBS api under development
void mir_presentation_chain_submit_buffer(MirPresentationChain* chain, MirBuffer* buffer)
try
{
    mir::require(chain && buffer && mir_presentation_chain_is_valid(chain));
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

void mir_connection_create_presentation_chain(
    MirConnection* connection, mir_presentation_chain_callback callback, void* context)
try
{
    mir::require(connection);
    connection->create_presentation_chain(callback, context);
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

MirPresentationChain* mir_connection_create_presentation_chain_sync(MirConnection* connection)
{
    ChainResult result;
    mir_connection_create_presentation_chain(connection,
        reinterpret_cast<mir_presentation_chain_callback>(set_result), &result);

    return result.wait_for_result();
}

void mir_presentation_chain_release(MirPresentationChain* chain)
try
{
    mir::require(chain);
    chain->connection()->release_presentation_chain(chain);
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}
