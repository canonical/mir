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
#include "buffer.h"

namespace mcl = mir::client;
//private NBS api under development
bool mir_presentation_chain_submit_buffer(MirPresentationChain*, MirBuffer* buffer)
{
    auto b = reinterpret_cast<mcl::MirBufferStub*>(buffer);
    b->ready();
    return true;
}

bool mir_presentation_chain_is_valid(MirPresentationChain*)
{
    return true;
}

char const *mir_presentation_chain_get_error_message(MirPresentationChain*)
{
    return "";
}

MirWaitHandle* mir_connection_create_presentation_chain(MirConnection*, mir_presentation_chain_callback, void*)
{
    return nullptr;
}

MirPresentationChain* mir_connection_create_presentation_chain_sync(MirConnection*)
{
    return nullptr;
}

void mir_presentation_chain_release(MirPresentationChain*)
{
}
