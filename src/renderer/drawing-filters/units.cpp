// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Utilities for handling coordinate system transformations in filters
 *
 * Author:
 *   Niko Kiirala <niko@kiirala.com>
 *
 * Copyright (C) 2007 Niko Kiirala
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <glib.h>
#include <2geom/affine.h>
#include <2geom/rect.h>
#include <2geom/transforms.h>

#include "units.h"

using Geom::X;
using Geom::Y;

namespace Inkscape::Renderer::DrawingFilter {

Units::Units()
    : filterUnits(SP_FILTER_UNITS_OBJECTBOUNDINGBOX)
    , primitiveUnits(SP_FILTER_UNITS_USERSPACEONUSE)
    , resolution_x(-1)
    , resolution_y(-1)
    , paraller_axis(false)
    , automatic_resolution(true)
{}

Units::Units(SPFilterUnits const filterUnits, SPFilterUnits const primitiveUnits)
    : filterUnits(filterUnits), primitiveUnits(primitiveUnits)
    , resolution_x(-1)
    , resolution_y(-1)
    , paraller_axis(false)
    , automatic_resolution(true)
{}

void Units::set_ctm(Geom::Affine const &ctm) {
    this->ctm = ctm;
}

void Units::set_resolution(double const x_res, double const y_res) {
    g_assert(x_res > 0);
    g_assert(y_res > 0);

    resolution_x = x_res;
    resolution_y = y_res;
}

void Units::set_paraller(bool const paraller) {
    paraller_axis = paraller;
}

void Units::set_automatic_resolution(bool const automatic) {
    automatic_resolution = automatic;
}

Geom::Affine Units::get_matrix_user2pb() const {
    if (resolution_x <= 0 || resolution_y <= 0 || !filter_area) {
        return Geom::identity();
    }

    Geom::Affine u2pb = ctm;

    if (paraller_axis || !automatic_resolution) {
        u2pb[0] = resolution_x / filter_area->width();
        u2pb[1] = 0;
        u2pb[2] = 0;
        u2pb[3] = resolution_y / filter_area->height();
        u2pb[4] = ctm[4];
        u2pb[5] = ctm[5];
    }

    return u2pb;
}

Geom::Affine Units::get_matrix_units2pb(SPFilterUnits units) const {
    if ( item_bbox && (units == SP_FILTER_UNITS_OBJECTBOUNDINGBOX) ) {

        Geom::Affine u2pb = get_matrix_user2pb();

        /* TODO: make sure that user coordinate system (0,0) is in correct
         * place in pixblock coordinates */
        Geom::Scale scaling(item_bbox->width(), item_bbox->height());
        u2pb *= scaling;
        return u2pb;

    } else if (units == SP_FILTER_UNITS_USERSPACEONUSE) {
        return get_matrix_user2pb();
    } else {
        g_warning("Error in Inkscape::Filters::Units::get_matrix_units2pb: unrecognized unit type (%d)", units);
        return Geom::Affine();
    }
}

Geom::Affine Units::get_matrix_filterunits2pb() const {
    return get_matrix_units2pb(filterUnits);
}

Geom::Affine Units::get_matrix_primitiveunits2pb() const {
    return get_matrix_units2pb(primitiveUnits);
}

Geom::Affine Units::get_matrix_display2pb() const {
    Geom::Affine d2pb = ctm.inverse();
    d2pb *= get_matrix_user2pb();
    return d2pb;
}

std::optional<Geom::Affine> Units::get_matrix_item2filter() const
{
    auto trans = get_matrix_display2pb();
    auto sbox = get_slot_box();
    if (render_area && sbox && !trans.isTranslation()) {
        // We don't know why we shift by -sbox and then render_area these would
        // be likely to be the same, if you know please update this comment
        return Geom::Translate(render_area->min()) * trans * Geom::Translate(-sbox->min());
    }
    return {};
}

Geom::Affine Units::get_matrix_pb2display() const {
    Geom::Affine pb2d = get_matrix_user2pb().inverse();
    pb2d *= ctm;
    return pb2d;
}

Geom::Affine Units::get_matrix_user2units(SPFilterUnits units) const {
    if (item_bbox && units == SP_FILTER_UNITS_OBJECTBOUNDINGBOX) {
        /* No need to worry about rotations: bounding box coordinates
         * always have base vectors paraller with userspace coordinates */
        Geom::Point min(item_bbox->min());
        Geom::Point max(item_bbox->max());
        double scale_x = 1.0 / (max[X] - min[X]);
        double scale_y = 1.0 / (max[Y] - min[Y]);
        //return Geom::Translate(min) * Geom::Scale(scale_x,scale_y); ?
        return Geom::Affine(scale_x, 0,
                            0, scale_y,
                            min[X] * scale_x, min[Y] * scale_y);
    } else if (units == SP_FILTER_UNITS_USERSPACEONUSE) {
        return Geom::identity();
    } else {
        g_warning("Error in Inkscape::Filters::Units::get_matrix_user2units: unrecognized unit type (%d)", units);
        return Geom::Affine();
    }
}

Geom::Affine Units::get_matrix_user2filterunits() const {
    return get_matrix_user2units(filterUnits);
}

Geom::Affine Units::get_matrix_user2primitiveunits() const {
    return get_matrix_user2units(primitiveUnits);
}

Geom::IntRect Units::get_pixblock_filterarea_paraller() const {
    g_assert(filter_area);

    Geom::Affine u2pb = get_matrix_user2pb();
    Geom::Rect r = *filter_area * u2pb;
    Geom::IntRect ir = r.roundOutwards();
    return ir;
}

Geom::OptRect Units::get_slot_box() const
{
    if (!render_area) {
        return {};
    }

    // compute slot bbox
    Geom::Affine trans = get_matrix_display2pb();
    Geom::Rect bbox_trans = *render_area * trans;
    Geom::Point min = bbox_trans.min();

    if (trans.isTranslation()) {
        // This looks like it would return exactly the same thing as below
        // but we're keeping this cruft until we have tests in place
        return Geom::Rect::from_xywh(min[X], min[Y],
            std::ceil(render_area->width()),
            std::ceil(render_area->height())
        );
    }
    return Geom::Rect::from_xywh(
        min[X], min[Y],
        std::ceil(bbox_trans.width()),
        std::ceil(bbox_trans.height())
    );
}


Units& Units::operator=(Units const &other) = default;

} // namespace Inkscape::Renderer::Filters

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
