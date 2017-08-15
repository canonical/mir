/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "connector_report.h"

namespace mrn = mir::report::null;

void mrn::ConnectorReport::thread_start() {}
void mrn::ConnectorReport::thread_end() {}
void mrn::ConnectorReport::creating_session_for(int /*socket_handle*/) {}
void mrn::ConnectorReport::creating_socket_pair(int /*server_handle*/, int /*client_handle*/) {}
void mrn::ConnectorReport::listening_on(std::string const& /*endpoint*/) {}
void mrn::ConnectorReport::error(std::exception const& /*error*/) {}
void mrn::ConnectorReport::warning(std::string const& /*error*/) {}

