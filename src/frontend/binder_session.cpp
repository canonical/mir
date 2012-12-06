/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "binder_session.h"

#include <binder/Parcel.h>
#include <utils/String8.h>

#include <sstream>
#include <stdexcept>

namespace mfd = mir::frontend::detail;

mfd::BinderSession::BinderSession() :
    processor(std::make_shared<NullMessageProcessor>()),
    response(0)
{
}

mfd::BinderSession::~BinderSession()
{
}

void mfd::BinderSession::set_processor(std::shared_ptr<MessageProcessor> const& processor)
{
    this->processor = processor;
}

void mfd::BinderSession::send(const std::ostringstream& buffer)
{
    assert(response);

    auto const& as_str(buffer.str());

    response->writeString8(android::String8(as_str.data(), as_str.length()));
}

void mfd::BinderSession::send_fds(std::vector<int32_t> const& fds)
{
    assert(response);

    for(auto fd: fds)
        response->writeFileDescriptor(fd, true);
}


android::status_t mfd::BinderSession::onTransact(
    uint32_t /*code*/,
    const android::Parcel& request,
    android::Parcel* response,
    uint32_t /*flags*/)
{
    this->response = response;

    // TODO there's probably a way to refactor this to avoid copy
    auto const& buffer = request.readString8();
    std::string inefficient_copy(buffer.string(), buffer.string()+buffer.size());
    std::istringstream msg(inefficient_copy);

    // TODO if this returns false, must close BinderSession
    processor->process_message(msg);

    assert(response == this->response);
    this->response = 0;
    return android::OK;
}

#include <iostream> // DEBUG
android::status_t mfd::BinderSession::pingBinder()
{
    std::cerr << "DEBUG: " << __PRETTY_FUNCTION__ << std::endl;
    return android::OK;
}
