/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2 or 3 as
 * published by the Free Software Foundation.
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

#ifndef MIR_CLIENT_MIR_PRESENTATION_CHAIN_H
#define MIR_CLIENT_MIR_PRESENTATION_CHAIN_H

#include "mir/geometry/size.h"
#include "mir_toolkit/mir_presentation_chain.h"
#include "mir/mir_buffer.h"

class MirPresentationChain
{
public:
    virtual ~MirPresentationChain() = default;
    virtual void submit_buffer(mir::client::MirBuffer* buffer) = 0;
    virtual MirConnection* connection() const = 0;
    virtual int rpc_id() const = 0;
    virtual char const* error_msg() const = 0;

    //In the future, the only mode will be dropping
    virtual void set_dropping_mode() = 0;
    virtual void set_queueing_mode() = 0;

protected:
    MirPresentationChain(MirPresentationChain const&) = delete;
    MirPresentationChain& operator=(MirPresentationChain const&) = delete;
    MirPresentationChain() = default;
};

#endif /* MIR_CLIENT_MIR_PRESENTATION_CHAIN_H */
