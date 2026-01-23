// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * SVG DropShadow primitive filter
 *
 * Authors:
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2026 AUTHORS
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "drop-shadow.h"
#include "slot.h"

#include "colors/color.h"

#include "renderer/context.h"
#include "renderer/pixel-filters/gaussian-blur.h"
#include "renderer/surface.h"

namespace Inkscape::Renderer::DrawingFilter {

void DropShadow::render(Slot &slot) const
{
    auto color = _color ? *_color : Colors::Color(0xff);

    auto in = slot.get(_input, _color_space);
    if (!in) {
        g_warning("Drop shaddow requires input texture, skipping.");
        return;
    }

    auto out = in->similar();
    auto offset = _offset;

    {
        auto context = Context(*out);
        context.transform(Geom::Translate(offset));
        context.setSource(color);
        context.mask(*in);
    }

    slot.set(_output, out);
    out = _render(slot, _output);

    {
        auto context = Context(*out);
        context.setSource(*in);
        context.paint();
    }

    slot.set(_output, out);
}

void DropShadow::set_color(Colors::Color color)
{
    _color = color;
}

bool DropShadow::can_handle_affine(Geom::Affine const &) const
{
    return true;
}

void DropShadow::area_enlarge(Geom::IntRect &area, Geom::Affine const &trans) const
{
    Geom::IntRect old = area;
    auto offset = _offset * Geom::Scale(trans.expansionX(), trans.expansionY());
    GaussianBlur::area_enlarge(area, trans * Geom::Translate(offset));
    area.unionWith(old);
}

} // namespace Inkscape::Renderer::DrawingFilter

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
