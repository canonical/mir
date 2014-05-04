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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "mir/scene/observer.h"

namespace ms = mir::scene;

void ms::Observer::surface_added(ms::Surface* /* surface */) {}
void ms::Observer::surface_removed(ms::Surface* /* surface */) {}
void ms::Observer::surfaces_reordered() {}
void ms::Observer::surface_exists(ms::Surface* /* surface */) {}
void ms::Observer::end_observation() {}
