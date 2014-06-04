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

#ifndef MIR_SCENE_TRUST_SESSION_IMPL_H_
#define MIR_SCENE_TRUST_SESSION_IMPL_H_

#include "mir/scene/trust_session.h"

namespace mir
{
namespace scene
{
class TrustSessionImpl : public TrustSession
{
public:
    explicit TrustSessionImpl(std::weak_ptr<Session> const& session);

    std::weak_ptr<Session> get_trusted_helper() const override;

protected:
    TrustSessionImpl(const TrustSessionImpl&) = delete;
    TrustSessionImpl& operator=(const TrustSessionImpl&) = delete;

private:
    std::weak_ptr<Session> const trusted_helper;
};
}
}

#endif // MIR_SCENE_TRUST_SESSION_IMPL_H_
