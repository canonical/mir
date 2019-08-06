/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/scene/application_not_responding_detector_wrapper.h"

namespace ms = mir::scene;

ms::ApplicationNotRespondingDetectorWrapper::ApplicationNotRespondingDetectorWrapper(
    std::shared_ptr<ApplicationNotRespondingDetector> const& wrapped) :
    wrapped{wrapped}
{
}

ms::ApplicationNotRespondingDetectorWrapper::~ApplicationNotRespondingDetectorWrapper() = default;

void ms::ApplicationNotRespondingDetectorWrapper::register_session(scene::Session const* session, std::function<void()> const& pinger)
{
    wrapped->register_session(session, pinger);
}

void ms::ApplicationNotRespondingDetectorWrapper::unregister_session(scene::Session const* session)
{
    wrapped->unregister_session(session);
}

void ms::ApplicationNotRespondingDetectorWrapper::pong_received(scene::Session const* received_for)
{
    wrapped->pong_received(received_for);
}

void ms::ApplicationNotRespondingDetectorWrapper::register_observer(std::shared_ptr<Observer> const& observer)
{
    wrapped->register_observer(observer);
}

void ms::ApplicationNotRespondingDetectorWrapper::unregister_observer(std::shared_ptr<Observer> const& observer)
{
    wrapped->unregister_observer(observer);
}
