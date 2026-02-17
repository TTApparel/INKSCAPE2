// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * feMorphology filter primitive renderer
 *
 * Authors:
 *   Felipe Corrêa da Silva Sanches <juca@members.fsf.org>
 *
 * Copyright (C) 2007 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "morphology.h"
#include "slot.h"

#include "renderer/pixel-filters/morphology.h"
#include "renderer/surface.h"

namespace Inkscape::Renderer::DrawingFilter {

void Morphology::render(Slot &slot) const
{
    auto input = slot.get(_input, _color_space);

    if (xradius == 0.0 || yradius == 0.0) {
        // output is transparent black
        slot.set(_output, input->similar());
        return;
    }

    int device_scale = slot.get_drawing_options().device_scale;
    Geom::Affine p2pb = slot.get_item_options().get_matrix_primitiveunits2pb();
    auto radius = Geom::Point(xradius, yradius) * p2pb * device_scale;

    auto mid = input->similar();
    auto out = mid->similar();
    out->run_pixel_filter<PixelAccessEdgeMode::NO_CHECK,
                          PixelAccessEdgeMode::ZERO,
                          PixelAccessEdgeMode::ZERO>(
        PixelFilter::Morphology(Operator == MorphologyOperator::ERODE, radius), *mid, *input);

    slot.set(_output, out);
}

void Morphology::area_enlarge(Geom::IntRect &area, Geom::Affine const &trans) const
{
    int enlarge_x = std::ceil(xradius * trans.expansionX());
    int enlarge_y = std::ceil(yradius * trans.expansionY());

    area.expandBy(enlarge_x, enlarge_y);
}

double Morphology::complexity(Geom::Affine const &trans) const
{
    int enlarge_x = std::ceil(xradius * trans.expansionX());
    int enlarge_y = std::ceil(yradius * trans.expansionY());
    return enlarge_x * enlarge_y;
}

void Morphology::set_operator(MorphologyOperator o)
{
    Operator = o;
}

void Morphology::set_xradius(double x)
{
    xradius = x;
}

void Morphology::set_yradius(double y)
{
    yradius = y;
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
