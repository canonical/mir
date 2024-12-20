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

#include <chrono>
#include <functional>
#include <memory>
#include <unordered_set>

namespace mir
{
class MainLoop;
namespace time { class Alarm; }
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
            std::optional<RevocationListener> revocation_listener);

        bool operator==(Token const& token) const;
        operator std::string() const;
        std::size_t hash() const;

    private:
        friend TokenAuthority;

        explicit Token(std::string const&);

        std::string token;
        std::shared_ptr<mir::time::Alarm> const alarm;
        std::optional<RevocationListener> revocation_listener;
    };

    TokenAuthority(std::shared_ptr<MainLoop> main_loop);

    auto issue_token(std::optional<Token::RevocationListener> revocation_listener) -> Token;

    void revoke_token(std::string to_remove);

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
