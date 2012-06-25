/*
 * Copyright © 2012 Canonical Ltd.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Canonical Ltd. not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 * Canonical Ltd. makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * CANONICAL, LTD. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL CANONICAL, LTD. BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef COMPOSITOR_H_
#define COMPOSITOR_H_

#include "drawer.h"

namespace mir
{
namespace surfaces
{
// scenegraph is the interface compositor uses onto the surface stack
class scenegraph;
}

namespace compositor
{
class buffer_texture_binder;

class compositor : public drawer
{
public:
    explicit compositor(
        surfaces::scenegraph* scenegraph,
        buffer_texture_binder* buffermanager);

    virtual void render(graphics::display* display);

private:
    surfaces::scenegraph* const scenegraph;
    buffer_texture_binder* const buffermanager;
};


}
}

#endif /* COMPOSITOR_H_ */
