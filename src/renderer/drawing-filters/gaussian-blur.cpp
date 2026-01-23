// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2026 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "renderer/drawing-filters/gaussian-blur.h"
#include "renderer/pixel-filters/gaussian-blur.h"
#include "renderer/surface.h"
#include "renderer/context.h"

#include "util/fixed_point.h"

#include "slot.h"
#include "enums.h"

namespace Inkscape::Renderer::DrawingFilter {

static int _effect_area_scr(double const deviation)
{
    return (int)std::ceil(std::fabs(deviation) * 3.0);
}

void GaussianBlur::render(Slot &slot) const
{
    // zero deviation = no change in output
    if (_deviation_x > 0 && _deviation_y > 0) {
        slot.set(_output, _render(slot, _input));
    }
    slot.set(_output, slot.get(_input));
}

std::shared_ptr<Surface> GaussianBlur::_render(Slot &slot, int input) const
{
    auto src = slot.get(input);

    // Handle bounding box case.
    auto &item_opt = slot.get_item_options();
    double dx = _deviation_x;
    double dy = _deviation_y;
    if( item_opt.get_primitive_units() == SP_FILTER_UNITS_OBJECTBOUNDINGBOX ) {
        Geom::OptRect const bbox = item_opt.get_item_bbox();
        if( bbox ) {
            dx *= (*bbox).width();
            dy *= (*bbox).height();
        }
    }

    Geom::Affine trans = item_opt.get_matrix_user2pb();
    int device_scale = slot.get_drawing_options().device_scale;

    Geom::Point deviation(dx * trans.expansionX() * device_scale,
                          dy * trans.expansionY() * device_scale);
    Geom::IntPoint size = src->dimensions();
    Geom::Point old = size;

    PixelFilter::GaussianBlur::downsampleForQuality(slot.get_drawing_options().blurquality, size, deviation);

    std::shared_ptr<Surface> dest;
    auto tr = Geom::Scale(size[Geom::X] / old[Geom::X], size[Geom::Y] / old[Geom::Y]);
    if (tr != Geom::identity()) {
        // Don't copy as we need to resize for this blurquality
        src = slot.get(input, _color_space);
        dest = src->similar(size);
        auto context = Context(*dest);
        context.transform(tr);
        context.setSource(*src);
        context.setOperator(Cairo::Context::Operator::SOURCE);
        context.paint();
    } else {
        // No resizing, just get the source in the color space as a copy
        dest = slot.get_copy(input, _color_space);
    }
    if (!dest) {
        std::cerr << "No source slot found in gaussian blur.\n";
        return {};
    }

    dest->run_pixel_filter<PixelAccessEdgeMode::ZERO>(PixelFilter::GaussianBlur(deviation));

    if (tr == Geom::identity()) {
        return dest;
    }

    // Resize it back if we need to
    auto context = Context(*src);
    context.transform(tr.inverse());
    context.setSource(*dest);
    context.setOperator(Cairo::Context::Operator::SOURCE);
    context.paint();
    return src;
}

void GaussianBlur::area_enlarge(Geom::IntRect &area, Geom::Affine const &trans) const
{
    int area_x = _effect_area_scr(_deviation_x * trans.expansionX());
    int area_y = _effect_area_scr(_deviation_y * trans.expansionY());
    // maximum is used because rotations can mix up these directions
    // TODO: calculate a more tight-fitting rendering area
    int area_max = std::max(area_x, area_y);
    area.expandBy(area_max);
}

bool GaussianBlur::can_handle_affine(Geom::Affine const &) const
{
    // Previously we tried to be smart and return true for rotations.
    // However, the transform passed here is NOT the total transform
    // from filter user space to screen.
    // TODO: fix this, or replace can_handle_affine() with isotropic().
    return false;
}

double GaussianBlur::complexity(Geom::Affine const &trans) const
{
    int area_x = _effect_area_scr(_deviation_x * trans.expansionX());
    int area_y = _effect_area_scr(_deviation_y * trans.expansionY());
    return 2.0 * area_x * area_y;
}

void GaussianBlur::set_deviation(double deviation)
{
    if (std::isfinite(deviation) && deviation >= 0) {
        _deviation_x = _deviation_y = deviation;
    }
}

void GaussianBlur::set_deviation(double x, double y)
{
    if (std::isfinite(x) && x >= 0 && std::isfinite(y) && y >= 0) {
        _deviation_x = x;
        _deviation_y = y;
    }
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
