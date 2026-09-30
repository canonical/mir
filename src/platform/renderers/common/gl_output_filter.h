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

#ifndef MIR_RENDERER_COMMON_GL_OUTPUT_FILTER_H_
#define MIR_RENDERER_COMMON_GL_OUTPUT_FILTER_H_

#include "gl_handles.h"

#include <mir/geometry/size.h>
#include <mir_toolkit/common.h>

#include <memory>

namespace mir::renderer::common
{

/// A whole-output color transformation, applied after the scene is composited.
///
/// When a filter is active the scene is drawn into an intermediate texture
/// instead of the real target, and `apply()` then draws that texture into the
/// real target through the filter's fragment shader. With no filter active this
/// does nothing, and the caller draws into its target directly.
class GLOutputFilter
{
public:
    /// \note Must be constructed with a current GL context
    GLOutputFilter();
    ~GLOutputFilter();

    GLOutputFilter(GLOutputFilter const&) = delete;
    auto operator=(GLOutputFilter const&) -> GLOutputFilter& = delete;

    void set_filter(MirOutputFilter filter);

    /// Whether drawing needs to be redirected through this filter
    auto active() const -> bool;

    /// Bind the intermediate framebuffer the scene should be drawn into
    ///
    /// \param   size  The size of the output being drawn into
    /// \return  false if no filter is active, in which case the caller should
    ///          bind its own target and skip `apply()`.
    auto bind_intermediate(geometry::Size size) -> bool;

    /// Draw the composited scene into the currently bound framebuffer
    ///
    /// \note The caller must bind its real target before calling this.
    void apply();

private:
    void ensure_texture_storage_for(geometry::Size size);

    TextureHandle const texture;
    FramebufferHandle const framebuffer;
    geometry::Size texture_size;
    MirOutputFilter filter{mir_output_filter_none};
    std::unique_ptr<ProgramHandle> program;
    GLint position_attrib{0};
    GLint texcoord_attrib{0};
    GLint tex_uniform{0};
};

}

#endif // MIR_RENDERER_COMMON_GL_OUTPUT_FILTER_H_
