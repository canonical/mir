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
 * Authored by: Kevin DuBois<kevin.dubois@canonical.com>
 */

#include "mir_client/mir_client_library.h"
#include "mir_client/mir_connection.h"
#include "mir_client/gbm/gbm_client_buffer_depository.h"

namespace mcl=mir::client;
namespace geom=mir::geometry;

std::shared_ptr<mcl::ClientBufferDepository> mcl::create_platform_depository()
{
    return std::make_shared<mcl::GBMClientBufferDepository>();
}
