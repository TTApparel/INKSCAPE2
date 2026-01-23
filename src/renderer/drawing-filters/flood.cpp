// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * feFlood filter primitive renderer
 *
 * Authors:
 *   Felipe Corrêa da Silva Sanches <juca@members.fsf.org>
 *   Tavmjong Bah <tavmjong@free.fr> (use primitive filter region)
 *
 * Copyright (C) 2007, 2011 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "renderer/context.h"
#include "renderer/surface.h"

#include "flood.h"
#include "slot.h"
#include "units.h"

namespace Inkscape::Renderer::DrawingFilter {

Flood::Flood()
    : color(0x00000000)
{}

void Flood::render(Slot &slot) const
{
    auto input = slot.get(_input);
    auto out = input->similar({}, _color_space);

    // Get filter primitive area in user units
    Geom::Rect fp = filter_primitive_area(slot.get_item_options());

    // Convert to Cairo units
    Geom::Rect fp_cairo = fp * slot.get_item_options().get_matrix_user2pb();

    // Get area in slot (tile to fill)
    Geom::Rect sa = *slot.get_item_options().get_slot_box();

    // Get overlap
    auto optoverlap = intersect(fp_cairo, sa);
    if (optoverlap) {
        Geom::Rect overlap = *optoverlap;

        auto d = fp_cairo.min() - sa.min();
        if (d.x() < 0.0) d.x() = 0.0;
        if (d.y() < 0.0) d.y() = 0.0;

        auto ct = Renderer::Context(*out);
        ct.setSource(color);
        ct.setOperator(Cairo::Context::Operator::SOURCE);
        ct.rectangle(Geom::Rect::from_xywh(d.x(), d.y(), overlap.width(), overlap.height()));
        ct.fill();
    }

    slot.set(_output, std::move(out));
}

bool Flood::can_handle_affine(Geom::Affine const &) const
{
    // flood is a per-pixel primitive and is immutable under transformations
    return true;
}

void Flood::set_color(Colors::Color const &c)
{
    color = c;
}

double Flood::complexity(Geom::Affine const &) const
{
    // flood is actually less expensive than normal rendering,
    // but when flood is processed, the object has already been rendered
    return 1.0;
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
