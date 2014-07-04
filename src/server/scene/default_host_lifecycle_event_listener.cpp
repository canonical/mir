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
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#include "mir/scene/default_host_lifecycle_event_listener.h"
#include "session_container.h"
#include "mir/scene/session.h"

#include <stdio.h>
#include <assert.h>

namespace mir
{
namespace scene
{

DefaultHostLifecycleEventListener::DefaultHostLifecycleEventListener(
		std::shared_ptr<SessionContainer> const& container) :
		app_container(container)
{
    assert(container);
}

void DefaultHostLifecycleEventListener::occured(MirLifecycleState state)
{
	printf("Life cycle event occured : state = %d\n", state);

	app_container->for_each(
			[&state](std::shared_ptr<Session> const& session)
			{
				session->set_lifecycle_state(state);
			});
}

}
}
