/*
 * Copyright © 2021 Canonical Ltd.
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
 */

#include "miroil/setcompositor.h"
#include "miroil/compositor.h"
#include <stdexcept>

// mir
#include <mir/server.h>
#include <mir/shell/shell.h>
#include <mir/compositor/compositor.h>

namespace miroil {

struct SetCompositor::QtCompositorImpl : public mir::compositor::Compositor
{
    QtCompositorImpl(const std::shared_ptr<miroil::Compositor> & compositor) 
    : m_customCompositor(compositor)
    {
    }
    
    // QtCompositor, 
    std::shared_ptr<miroil::Compositor> getWrapped() { return m_customCompositor; };
    
    void start();
    void stop();
    
    std::shared_ptr<miroil::Compositor> m_customCompositor;
};

void SetCompositor::QtCompositorImpl::start()
{
    return m_customCompositor->start();
}

void SetCompositor::QtCompositorImpl::stop()
{
    return m_customCompositor->stop();
}

SetCompositor::SetCompositor(constructorFunction constr, initFunction init)
    : m_constructor(constr), m_init(init)
{
}

void SetCompositor::operator()(mir::Server& server)
{
    server.override_the_compositor([this]
    {
        auto result = std::make_shared<QtCompositorImpl>(m_constructor());
        m_compositor = result;
        return result;
    });

    server.add_init_callback([&, this]
        {
            if (auto const compositor = m_compositor.lock())
            {
                m_init(server.the_display(), compositor->getWrapped(), server.the_shell());
            }
            else
            {
                throw std::logic_error("No m_compositor available. Server not running?");
            }
        });
}

}
