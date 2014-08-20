/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored By: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "debugging_session_mediator.h"

#include "mir/frontend/session.h"
#include "../scene/basic_surface.h"

namespace ms = mir::scene;

void mir::frontend::DebuggingSessionMediator::translate_surface_to_screen(
    ::google::protobuf::RpcController* ,
    ::mir::protobuf::CoordinateTranslationRequest const* request,
    ::mir::protobuf::CoordinateTranslationResponse* response,
    ::google::protobuf::Closure *done)
{
    {
        std::unique_lock<std::mutex> lock(session_mutex);

        auto session = weak_session.lock();

        if (session.get() == nullptr)
            BOOST_THROW_EXCEPTION(std::logic_error("Invalid application session"));

        auto const id = frontend::SurfaceId(request->surfaceid().value());
        auto const surface = std::dynamic_pointer_cast<ms::BasicSurface>(session->get_surface(id));

        response->set_x(request->x() + surface->top_left().x.as_int());
        response->set_y(request->y() + surface->top_left().y.as_int());
    }
    done->Run();
}
