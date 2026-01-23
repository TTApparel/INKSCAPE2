// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * feImage filter primitive renderer
 *
 * Copyright (C) 2007-2026 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "renderer/context.h"
#include "renderer/surface.h"

#include "image.h"
#include "slot.h"
#include "units.h"

namespace Inkscape::Renderer::DrawingFilter {

void Image::render(Slot &slot) const
{
    if (!_item_box || !_render_function) {
        return;
    }

    // Viewport is filter primitive area (in user coordinates).
    // Note: viewport calculation in non-trivial. Do not rely
    // on get_matrix_primitiveunits2pb().
    Geom::Rect vp = filter_primitive_area(slot.get_item_options());
    slot.set_primitive_area(_output, vp); // Needed for tiling

    double feImageX      = vp.left();
    double feImageY      = vp.top();
    double feImageWidth  = vp.width();
    double feImageHeight = vp.height();

    // feImage is suppose to use the same parameters as a normal SVG image.
    // If a width or height is set to zero, the image is not suppose to be displayed.
    // This does not seem to be what Firefox or Opera does, nor does the W3C displacement
    // filter test expect this behavior. If the width and/or height are zero, we use
    // the width and height of the object bounding box.
    Geom::Affine m = slot.get_item_options().get_matrix_user2filterunits().inverse();
    Geom::Point bbox_00 = Geom::Point(0,0) * m;
    Geom::Point bbox_w0 = Geom::Point(1,0) * m;
    Geom::Point bbox_0h = Geom::Point(0,1) * m;
    double bbox_width = Geom::distance(bbox_00, bbox_w0);
    double bbox_height = Geom::distance(bbox_00, bbox_0h);

    if (feImageWidth  == 0) feImageWidth  = bbox_width;
    if (feImageHeight == 0) feImageHeight = bbox_height;

    int device_scale = slot.get_drawing_options().device_scale;

    Geom::Rect sa = *slot.get_item_options().get_slot_box();
    auto out = std::make_shared<Surface>(sa.dimensions().round(), device_scale, _color_space);

    Context dc(*out);
    Geom::Affine user2pb = slot.get_item_options().get_matrix_user2pb();
    dc.transform(user2pb); // we are now in primitive units

    Geom::IntRect render_rect = _item_box->roundOutwards();

    // Internal image, like <use>
    if (from_element) {
        dc.translate(Geom::Translate(feImageX, feImageY));
        _render_function(dc, slot.get_drawing_options(), render_rect);
    } else {
        // Now that we have the viewport, we must map image inside.
        // Partially copied from sp-image.cpp.

        auto image_width  = _item_box->width();
        auto image_height = _item_box->height();

        // Do nothing if preserveAspectRatio is "none".
        if (_align) {
            auto align = *_align;

            // Check aspect ratio of image vs. viewport
            double feAspect = feImageHeight / feImageWidth;
            double aspect = (double)image_height / image_width;
            bool ratio = feAspect < aspect;

            if (aspect_is_slice) {
                // image clipped by viewbox

                if (ratio) {
                    // clip top/bottom
                    feImageY -= align[Geom::Y] * (feImageWidth * aspect - feImageHeight);
                    feImageHeight = feImageWidth * aspect;
                } else {
                    // clip sides
                    feImageX -= align[Geom::X] * (feImageHeight / aspect - feImageWidth);
                    feImageWidth = feImageHeight / aspect;
                }

            } else {
                // image fits into viewbox

                if (ratio) {
                    // fit to height
                    feImageX += align[Geom::X] * (feImageWidth - feImageHeight / aspect );
                    feImageWidth = feImageHeight / aspect;
                } else {
                    // fit to width
                    feImageY += align[Geom::Y] * (feImageHeight - feImageWidth * aspect);
                    feImageHeight = feImageWidth * aspect;
                }
            }
        }

        double scaleX = feImageWidth / image_width;
        double scaleY = feImageHeight / image_height;

        dc.translate(Geom::Translate(feImageX, feImageY));
        dc.scale(Geom::Scale(scaleX, scaleY));
        _render_function(dc, slot.get_drawing_options(), render_rect);
    }

    slot.set(_output, out);
}

bool Image::can_handle_affine(Geom::Affine const &) const
{
    return true;
}

double Image::complexity(Geom::Affine const &) const
{
    // TODO: right now we cannot actually measure this in any meaningful way.
    return 1.1;
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
