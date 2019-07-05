/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Alberto Aguirre <alberto.aguirre@canonical.com>
 */

#ifndef MIR_PROTOBUF_DISPLAY_SERVER_DEBUG_H_
#define MIR_PROTOBUF_DISPLAY_SERVER_DEBUG_H_

#include <google/protobuf/stubs/callback.h>
#include "mir_protobuf.pb.h"

namespace mir
{
namespace protobuf
{
class DisplayServerDebug
{

public:
    virtual ~DisplayServerDebug() = default;

    virtual void translate_surface_to_screen(
        mir::protobuf::CoordinateTranslationRequest const* request,
        mir::protobuf::CoordinateTranslationResponse* response,
        google::protobuf::Closure* done) = 0;

protected:
    DisplayServerDebug() = default;
private:
    DisplayServerDebug(DisplayServerDebug const&) = delete;
    void operator=(DisplayServerDebug const&) = delete;
};
}
}

#endif

