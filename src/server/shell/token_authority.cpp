/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "mir/shell/token_authority.h"

#include "mir/main_loop.h"
#include "mir/time/alarm.h"

#include <uuid.h>
#include <mutex>

namespace msh = mir::shell;

msh::TokenAuthority::Token::Token(
    std::string token, std::unique_ptr<time::Alarm> alarm, std::optional<RevocationListener> revocation_listener) :
    token{token},
    alarm{std::move(alarm)},
    revocation_listener{revocation_listener}
{
    alarm->reschedule_in(timeout_ms);
}

bool mir::shell::TokenAuthority::Token::operator==(Token const& token) const
{
    return this->token == token.token;
}

msh::TokenAuthority::Token::operator std::string() const
{
    return token;
}

std::size_t msh::TokenAuthority::Token::hash() const
{
    return std::hash<std::string>{}(token);
}

msh::TokenAuthority::Token::Token(std::string const& token) :
    token{token}
{
}

mir::shell::TokenAuthority::TokenAuthority(std::shared_ptr<MainLoop> main_loop) :
    main_loop{main_loop}
{
}

auto mir::shell::TokenAuthority::issue_token(std::optional<Token::RevocationListener> revocation_listener) -> Token
{
    std::scoped_lock lock{mutex};

    auto const generated = generate_token();
    auto alarm = main_loop->create_alarm(
        [this, generated]()
        {
            revoke_token(generated);
        });

    auto const [iter, _] = issued_tokens.emplace(generated, std::move(alarm), revocation_listener);
    return *iter;
}

void mir::shell::TokenAuthority::revoke_token(std::string to_remove)
{
    std::scoped_lock lock{mutex};
    auto const iter = issued_tokens.find(Token{to_remove});
    if (iter != issued_tokens.end())
    {
        if (auto cb = iter->revocation_listener)
            (*cb)(*iter);
        issued_tokens.erase(iter);
    }
}

auto msh::TokenAuthority::generate_token() -> std::string
{
    uuid_t uuid;
    uuid_generate(uuid);
    return {reinterpret_cast<char*>(uuid), 4};
}

