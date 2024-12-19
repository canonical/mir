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

#ifndef MIR_SHELL_TOKEN_AUTHORITY_H_
#define MIR_SHELL_TOKEN_AUTHORITY_H_

#include "mir/main_loop.h"
#include "mir/time/alarm.h"

#include <chrono>
#include <cmath>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unistd.h>
#include <unordered_set>

namespace mir
{
namespace shell
{
class TokenAuthority
{
public:
    class Token
    {
    public:
        using RevocationListener = std::function<void(Token const& token)>;

        Token(
            std::string token,
            std::unique_ptr<time::Alarm> alarm,
            std::optional<RevocationListener> revocation_listener) :
            token{token},
            alarm{std::move(alarm)},
            revocation_listener{revocation_listener}
        {
            alarm->reschedule_in(timeout_ms);
        }

        explicit Token(std::string token) :
            token{token},
            alarm{nullptr}
        {
        }

        operator std::string() const
        {
            return token;
        }

        bool operator==(Token const& token) const
        {
            return this->token == token.token;
        }

        std::size_t hash() const
        {
            return std::hash<std::string>{}(token);
        }

    private:
        friend TokenAuthority;
        std::string token;
        std::shared_ptr<mir::time::Alarm> const alarm;
        std::optional<RevocationListener> revocation_listener;
    };

    TokenAuthority(std::shared_ptr<MainLoop> main_loop) :
        main_loop{main_loop}
    {
    }

    auto issue_token(std::optional<Token::RevocationListener> revocation_listener) -> Token
    {
        std::scoped_lock lock{mutex};

        auto const generated = generate_token();
        auto alarm = main_loop->create_alarm(
            [this, generated]()
            {
                revoke_token(generated);
            });

        auto const [iter, _] = issued_tokens.insert(Token{generated ,std::move(alarm), revocation_listener});
        return *iter;
    }

    void revoke_token(std::string to_remove)
    {
        std::scoped_lock lock{mutex};
        auto const iter = issued_tokens.find(Token{to_remove});
        if(iter != issued_tokens.end())
        {
            if(auto cb = iter->revocation_listener)
                (*cb)(*iter);
            issued_tokens.erase(iter);
        }
    }

private:
    static auto generate_token() -> std::string;

    struct TokenHash
    {
        std::size_t operator()(mir::shell::TokenAuthority::Token const& s) const noexcept
        {
            return s.hash();
        }
    };
    std::unordered_set<Token, TokenHash> issued_tokens;

    std::shared_ptr<MainLoop> main_loop;
    std::mutex mutable mutex;

    /// Time in milliseconds that the compositor will wait before invalidating a token
    static auto constexpr timeout_ms = std::chrono::seconds(3000);
};
}
}


#endif
