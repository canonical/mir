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

        bool operator==(Token const& token) const;
        explicit operator std::string() const;

    private:
        friend TokenAuthority;

        Token(std::string token, std::optional<RevocationListener> revocation_listener);

        std::string token;
        std::optional<RevocationListener> revocation_listener;
    };

    explicit TokenAuthority(std::shared_ptr<MainLoop>&& main_loop);

    auto issue_token(std::optional<Token::RevocationListener> revocation_listener) -> Token;
    auto get_token_for_string(std::string const& string_token) const -> std::optional<Token>;
    auto get_bogus_token() const -> Token;

    void revoke_token(Token to_remove);
    void revoke_all_tokens();

private:
    static auto generate_token() -> std::string;

    using TimedToken = std::pair<Token, std::unique_ptr<time::Alarm>>;
    struct TimedTokenHash
    {
        std::size_t operator()(TimedToken const& s) const noexcept;
    };
    struct TimedTokenEq
    {
        bool operator()(TimedToken const& s1, TimedToken const& s2) const noexcept;
    };
    std::mutex mutable mutex;
    std::unordered_set<TimedToken, TimedTokenHash, TimedTokenEq> issued_tokens;

    std::shared_ptr<MainLoop> const main_loop;
};
}
}

#endif
