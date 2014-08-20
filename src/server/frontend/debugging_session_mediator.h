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

#ifndef MIR_FRONTEND_DEBUGGING_SESSION_MEDIATOR_H_
#define MIR_FRONTEND_DEBUGGING_SESSION_MEDIATOR_H_

#include "session_mediator.h"
#include "mir_protobuf.pb.h"

namespace mir
{
namespace frontend
{
class DebuggingSessionMediator : public SessionMediator, public mir::protobuf::Debug
{
public:
    using SessionMediator::SessionMediator;

    void translate_surface_to_screen(
        ::google::protobuf::RpcController* controller,
        ::mir::protobuf::CoordinateTranslationRequest const* request,
        ::mir::protobuf::CoordinateTranslationResponse* response,
        ::google::protobuf::Closure *done) override;
};

}
}


#endif // MIR_FRONTEND_DEBUGGING_SESSION_MEDIATOR_H_
