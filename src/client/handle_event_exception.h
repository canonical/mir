/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Brandon Schaefer <brandon.schaefer@canonical.com>
 */

#include "mir/log.h"

#define MIR_HANDLE_EVENT_EXCEPTION(expr) \
    try                                  \
    {                                    \
        expr;                            \
    }                                    \
    catch(std::exception const& e)       \
    {                                    \
        mir::log_critical(e.what());     \
        abort();                         \
    }
