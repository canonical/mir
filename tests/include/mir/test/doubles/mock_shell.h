/*
 * Copyright © 2013-2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_SHELL_H_
#define MIR_TEST_DOUBLES_SHELL_H_

#include "mir/frontend/shell.h"

#include "mir/scene/surface_creation_parameters.h"
#include "mir/scene/prompt_session_creation_parameters.h"
#include "mir/shell/surface_specification.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

struct MockShell : public frontend::Shell
{
    MOCK_METHOD3(open_session, std::shared_ptr<frontend::MirClientSession>(
        pid_t client_pid,
        std::string const&,
        std::shared_ptr<frontend::EventSink> const&));

    MOCK_METHOD1(close_session, void(std::shared_ptr<frontend::MirClientSession> const&));

    MOCK_METHOD2(start_prompt_session_for, std::shared_ptr<frontend::PromptSession>(
        std::shared_ptr<scene::Session> const&,
        scene::PromptSessionCreationParameters const&));
    MOCK_METHOD2(add_prompt_provider_for, void(
        std::shared_ptr<frontend::PromptSession> const&,
        std::shared_ptr<scene::Session> const&));
    MOCK_METHOD1(stop_prompt_session, void(std::shared_ptr<frontend::PromptSession> const&));

    MOCK_METHOD3(create_surface,
        frontend::SurfaceId(
            std::shared_ptr<frontend::MirClientSession> const&,
            scene::SurfaceCreationParameters const& params,
            std::shared_ptr<frontend::EventSink> const&));
    MOCK_METHOD3(modify_surface, void(std::shared_ptr<frontend::MirClientSession> const&, frontend::SurfaceId, shell::SurfaceSpecification const&));
    MOCK_METHOD2(destroy_surface, void(std::shared_ptr<frontend::MirClientSession> const&, frontend::SurfaceId));
    MOCK_METHOD2(persistent_id_for, std::string(std::shared_ptr<frontend::MirClientSession> const&, frontend::SurfaceId));
    MOCK_METHOD1(surface_for_id, std::shared_ptr<scene::Surface>(std::string const&));



    MOCK_METHOD4(set_surface_attribute, int(
        std::shared_ptr<frontend::MirClientSession> const& session, frontend::SurfaceId surface_id,
        MirWindowAttrib attrib, int value));

    MOCK_METHOD3(get_surface_attribute, int(std::shared_ptr<frontend::MirClientSession> const& session,
        frontend::SurfaceId surface_id, MirWindowAttrib attrib));

    MOCK_METHOD3(raise_surface, void(std::shared_ptr<frontend::MirClientSession> const& session,
        frontend::SurfaceId surface_id, uint64_t timestamp));

    MOCK_METHOD5(request_operation, void(std::shared_ptr<frontend::MirClientSession> const &session,
        frontend::SurfaceId surface_id, uint64_t timestamp, UserRequest request, optional_value<uint32_t> hint));
};

}
}
} // namespace mir

#endif // MIR_TEST_DOUBLES_SHELL_H_
