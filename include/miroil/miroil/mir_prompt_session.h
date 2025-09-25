/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIROIL_MIR_PROMPT_SESSION_H_
#define MIROIL_MIR_PROMPT_SESSION_H_
#include <memory>
#include <functional>

typedef struct MirPromptSession MirPromptSession;
typedef void (*MirClientFdCallback)(MirPromptSession *prompt_session, size_t count, int const* fds, void* context);

namespace miroil
{

class MirPromptSession
{
public:
    MirPromptSession(::MirPromptSession * promptSession);
    MirPromptSession(MirPromptSession const& src);
    MirPromptSession(MirPromptSession && src);
    ~MirPromptSession();

    auto operator=(MirPromptSession const& src) -> MirPromptSession&;
    auto operator=(MirPromptSession&& src) -> MirPromptSession&;

    bool operator==(MirPromptSession const& other);

    bool new_fds_for_prompt_providers(unsigned int no_of_fds, MirClientFdCallback callback, void * context);

    ::MirPromptSession * prompt_session;
};

}

#endif /* MIROIL_MIR_PROMPT_SESSION_H_ */
