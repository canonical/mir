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
 * Authored By: Nick Dedekind <nick.dedekind@canonical.com>
 */

#include "trusted_session_impl.h"
#include "mir/shell/trusted_session_creation_parameters.h"
#include "mir/scene/session_container.h"
#include "mir/scene/session.h"

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace msh = mir::shell;

ms::TrustedSessionImpl::TrustedSessionImpl(
    msh::TrustedSessionCreationParameters const& parameters) :
    applications(parameters.applications),
    started(false),
    next_session_id(0),
    current_id(next_id())
{
}

ms::TrustedSessionImpl::~TrustedSessionImpl()
{
    TrustedSessionImpl::stop();
}

mf::SessionId ms::TrustedSessionImpl::id() const
{
    return current_id;
}

mf::SessionId ms::TrustedSessionImpl::next_id()
{
    return mf::SessionId(next_session_id.fetch_add(1));
}

bool  ms::TrustedSessionImpl::get_started() const
{
    return started;
}

std::shared_ptr<msh::Session> ms::TrustedSessionImpl::get_trusted_helper() const
{
    return trusted_helper;
}

std::shared_ptr<msh::TrustedSession> ms::TrustedSessionImpl::create_for(std::shared_ptr<msh::Session> const& session,
                                                                        std::shared_ptr<SessionContainer> const& container,
                                                                        msh::TrustedSessionCreationParameters const& parameters)
{
    TrustedSessionImpl* impl = new TrustedSessionImpl(parameters);
    std::shared_ptr<msh::TrustedSession> ptr(impl);

    impl->trusted_helper = session;
    impl->started = true;
    session->set_trusted_session(ptr);
    auto scene_trusted_helper = std::dynamic_pointer_cast<ms::Session>(impl->trusted_helper);

    for (pid_t application_pid : impl->applications)
    {
        std::vector<std::shared_ptr<msh::Session>> added_sessions;

        container->for_each(
            [&]
            (std::shared_ptr<msh::Session> const& container_session)
            {
                if (container_session->process_id() == application_pid)
                {
                    auto scene_session = std::dynamic_pointer_cast<ms::Session>(session);

                    scene_session->set_parent(scene_trusted_helper);
                    scene_trusted_helper->get_children()->insert_session(container_session);

                    impl->add_child_session(container_session);
                    container_session->set_trusted_session(ptr);
                    added_sessions.push_back(container_session);
                }
            }
        );
    }

    return ptr;
}

void ms::TrustedSessionImpl::stop()
{
    if (!started)
        return;
    auto scene_session = std::dynamic_pointer_cast<ms::Session>(trusted_helper);

    scene_session->get_children()->for_each(
        [this]
        (std::shared_ptr<msh::Session> const& child_session)
        {
            auto scene_session = std::dynamic_pointer_cast<ms::Session>(child_session);
            scene_session->set_parent(NULL);
            child_session->set_trusted_session(NULL);
        }
    );
    scene_session->get_children()->clear();
    trusted_helper->set_trusted_session(NULL);

    started = false;
    trusted_helper = NULL;
}

std::vector<pid_t> ms::TrustedSessionImpl::get_applications() const
{
  return applications;
}

void ms::TrustedSessionImpl::add_child_session(std::shared_ptr<msh::Session> const& session)
{
    if (!started)
        return;

    printf("adding child session %s\n", session->name().c_str());

    auto scene_trusted_helper = std::dynamic_pointer_cast<ms::Session>(trusted_helper);
    auto scene_session = std::dynamic_pointer_cast<ms::Session>(session);

    scene_session->set_parent(scene_trusted_helper);
    scene_trusted_helper->get_children()->insert_session(session);
}
