// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * feOffset filter primitive renderer
 *
 * Authors:
 *   Niko Kiirala <niko@kiirala.com>
 *
 * Copyright (C) 2007 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "offset.h"

#include "slot.h"
#include "renderer/context.h"
#include "renderer/surface.h"

namespace Inkscape::Renderer::DrawingFilter {

void Offset::render(Slot &slot) const
{
    auto in = slot.get(_input, _color_space);
    auto out = in->similar();

    Geom::Rect vp = filter_primitive_area(slot.get_item_options());
    slot.set_primitive_area(_output, vp); // Needed for tiling

    Geom::Affine p2pb = slot.get_item_options().get_matrix_primitiveunits2pb();
    double x = dx * p2pb.expansionX();
    double y = dy * p2pb.expansionY();

    {
        auto ct = Context(*out);
        ct.setSource(*in, x, y);
        ct.paint();
    }
    slot.set(_output, out);
}

bool Offset::can_handle_affine(Geom::Affine const &) const
{
    return true;
}

void Offset::set_dx(double amount)
{
    dx = amount;
}

void Offset::set_dy(double amount)
{
    dy = amount;
}

void Offset::area_enlarge(Geom::IntRect &area, Geom::Affine const &trans) const
{
    Geom::Point offset(dx, dy);
    offset *= trans;
    offset[Geom::X] -= trans[4];
    offset[Geom::Y] -= trans[5];
    double x0, y0, x1, y1;
    x0 = area.left();
    y0 = area.top();
    x1 = area.right();
    y1 = area.bottom();

    if (offset[Geom::X] > 0) {
        x0 -= std::ceil(offset[Geom::X]);
    } else {
        x1 -= std::floor(offset[Geom::X]);
    }

    if (offset[Geom::Y] > 0) {
        y0 -= std::ceil(offset[Geom::Y]);
    } else {
        y1 -= std::floor(offset[Geom::Y]);
    }

    area = Geom::IntRect(x0, y0, x1, y1);
}

double Offset::complexity(Geom::Affine const &) const
{
    return 1.02;
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
