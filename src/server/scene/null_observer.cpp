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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "mir/scene/null_observer.h"

namespace ms = mir::scene;

void ms::NullObserver::surface_added(std::shared_ptr<ms::Surface> /* surface */) {}
void ms::NullObserver::surface_removed(std::shared_ptr<ms::Surface> /* surface */) {}
void ms::NullObserver::surfaces_reordered() {}
void ms::NullObserver::scene_changed() {}
void ms::NullObserver::surface_exists(std::shared_ptr<ms::Surface> /* surface */) {}
void ms::NullObserver::end_observation() {}
