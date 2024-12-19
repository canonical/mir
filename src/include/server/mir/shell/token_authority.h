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

#include <mutex>
#include <set>
#include <string>

namespace mir
{
namespace shell
{
class TokenAuthority
{
public:
    auto issue_token() -> std::string
    {
        std::scoped_lock lock{mutex};
        auto const [iter, _] = issued_tokens.insert(generate_token());
        return *iter;
    }

    auto verify_token(std::string const& token) const -> bool
    {
        std::scoped_lock lock{mutex};
        return issued_tokens.contains(token);
    }

    void revoke_token(std::string to_remove)
    {
        std::scoped_lock lock{mutex};
        issued_tokens.erase(to_remove);
    }

private:
    static auto generate_token() -> std::string;

    std::set<std::string> issued_tokens;
    std::mutex mutable mutex;
};
}
}

#endif
