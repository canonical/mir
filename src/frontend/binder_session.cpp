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
#include <stdexcept>

namespace mfd = mir::frontend::detail;

mfd::BinderSession::BinderSession() :
    processor(std::make_shared<NullMessageProcessor>())
{
}

mfd::BinderSession::~BinderSession()
{
}

void mfd::BinderSession::set_processor(std::shared_ptr<MessageProcessor> const& processor)
{
    this->processor = processor;
}

void mfd::BinderSession::send(const std::ostringstream& /*buffer2*/)
{
    // TODO
}

void mfd::BinderSession::send_fds(std::vector<int32_t> const& /*fd*/)
{
    // TODO
}


android::status_t mfd::BinderSession::onTransact(
    uint32_t /*code*/,
    const android::Parcel& /*request*/,
    android::Parcel* /*response*/,
    uint32_t /*flags*/)
{
    throw std::runtime_error("Not implemented");
}
