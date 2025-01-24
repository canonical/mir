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
#include "mir/log.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <mutex>
#include <uuid.h>

namespace
{
/// Time in milliseconds that the compositor will wait before invalidating a token
static auto constexpr timeout_ms = std::chrono::milliseconds(3000);
}

namespace msh = mir::shell;

msh::TokenAuthority::Token::Token(
    std::string token, std::optional<RevocationListener> revocation_listener) :
    token{token},
    revocation_listener{revocation_listener}
{
}

msh::TokenAuthority::Token::operator std::string() const
{
    return token;
}

bool msh::TokenAuthority::Token::operator==(Token const& token) const
{
    return this->token == token.token;
}

msh::TokenAuthority::TokenAuthority(std::shared_ptr<MainLoop>&& main_loop) :
    main_loop{std::move(main_loop)}
{
}

auto msh::TokenAuthority::issue_token(std::optional<Token::RevocationListener> revocation_listener) -> Token
{
    auto token = Token{generate_token(),  revocation_listener};

    auto alarm = main_loop->create_alarm(
        [this, token]()
        {
            revoke_token(token);
        });
    alarm->reschedule_in(::timeout_ms);

    {
        std::scoped_lock lock{mutex};

        revocation_alarms.emplace_back(std::move(alarm));
        issued_tokens.insert(token);
    }

    return token;
}

auto msh::TokenAuthority::get_token_for_string(std::string const& string_token) const -> std::optional<Token>
{
    std::scoped_lock lock{mutex};

    // Incoming string doesn't have null terminator, just like the one leaving Mir
    auto iter = issued_tokens.find(Token{string_token, {}});

    if (iter == issued_tokens.end())
        return std::nullopt;

    return *iter;
}

auto msh::TokenAuthority::is_token_valid(Token const& token) const -> bool
{
    std::scoped_lock lock{mutex};
    return issued_tokens.contains(token);
}

auto msh::TokenAuthority::get_bogus_token() const -> Token
{
    return Token{generate_token(), {}};
}

void msh::TokenAuthority::revoke_token(Token to_remove)
{
    std::scoped_lock lock{mutex};

    auto const iter = issued_tokens.find(Token{to_remove});
    if (iter != issued_tokens.end())
    {
        if (auto cb = iter->revocation_listener)
            (*cb)(*iter);
        issued_tokens.erase(iter);
    }

    // Remove the alarm that triggered this revocation, if any
    std::ranges::remove_if(
        revocation_alarms,
        [](auto const& alarm)
        {
            return alarm->state() == time::Alarm::triggered;
        });
}

void msh::TokenAuthority::revoke_all_tokens()
{
    std::scoped_lock guard{mutex};

    for (auto const& iter : issued_tokens)
    {
        if (auto cb = iter.revocation_listener; cb)
            (*cb)(iter);
    }

    issued_tokens.clear();
    revocation_alarms.clear();
}

auto msh::TokenAuthority::generate_token() -> std::string
{
    uuid_t uuid;
    uuid_generate(uuid);

    std::string unparsed(UUID_STR_LEN, '\0');
    uuid_unparse(uuid, unparsed.data());

    uuid_clear(uuid);

    // Don't need the null terminator
    return unparsed.substr(0, UUID_STR_LEN-1);
}

std::size_t msh::TokenAuthority::TokenHash::operator()(Token const& s) const noexcept
{
    return std::hash<std::string>{}(s.token);
}
