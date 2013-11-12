/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/frontend/session_mediator.h"
#include "mir/frontend/session_mediator_report.h"
#include "mir/frontend/session.h"
#include <boost/throw_exception.hpp>

#include <stdexcept>

void mir::frontend::SessionMediator::drm_auth_magic(
    google::protobuf::RpcController* /*controller*/,
    const mir::protobuf::DRMMagic* /*request*/,
    mir::protobuf::DRMAuthMagicStatus* /*response*/,
    google::protobuf::Closure* /*done*/)
{
    {
        std::unique_lock<std::mutex> lock(session_mutex);
        
        auto session = weak_session.lock();

        if (session.get() == nullptr)
            BOOST_THROW_EXCEPTION(std::logic_error("Invalid application session"));

        report->session_drm_auth_magic_called(session->name());
    }

    BOOST_THROW_EXCEPTION(std::logic_error("drm_auth_magic request not supported by the Android platform"));
}
