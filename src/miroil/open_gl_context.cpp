/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "miroil/open_gl_context.h"
#include <stdexcept>

// mir
#include <mir/server.h>

namespace miroil {
    
struct OpenGLContext::Self
{
    Self(mir::graphics::GLConfig * glConfig)
    : m_glConfig(glConfig) 
    {        
    }
    
    std::shared_ptr<mir::graphics::GLConfig> m_glConfig;
};

OpenGLContext::OpenGLContext(mir::graphics::GLConfig * glConfig)
:    self{std::make_shared<Self>(glConfig)}
{    
}

void OpenGLContext::operator()(mir::Server& server)
{
    server.override_the_gl_config([this]
        { return self->m_glConfig; }
    );
}

auto OpenGLContext::the_open_gl_config() const
-> std::shared_ptr<mir::graphics::GLConfig>
{
    return self->m_glConfig;
}
}
