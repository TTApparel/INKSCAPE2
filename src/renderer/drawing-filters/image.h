// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * feImage filter primitive renderer
 *
 * Copyright (C) 2007-2026 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_INKSCAPE_RENDERER_DRAWING_FILTER_IMAGE_H
#define SEEN_INKSCAPE_RENDERER_DRAWING_FILTER_IMAGE_H

#include <2geom/point.h>
#include <2geom/rect.h>
#include <memory>
#include <string>

#include "primitive.h"

namespace Inkscape::Renderer {

class Context;
class DrawingOptions;

namespace DrawingFilter {

/**
 * Using this callback allows us to test this component without inducing any of the complexity
 * in sp-filter-image or drawing-item which can be handled through this interface.
 */
using ImageRenderFunction = std::function<void(Context dc, DrawingOptions const &rc, Geom::IntRect const &area)>;

class Image : public Primitive
{
public:
    Image() = default;
    ~Image() override = default;

    void render(Slot &slot) const override;
    bool can_handle_affine(Geom::Affine const &) const override;
    double complexity(Geom::Affine const &ctm) const override;

    void set_href(char const *href);

    /**
     * This used to be the enum SP_ASPECT_, but this part of the stack shouldn't
     * really know how to translate this enum into an alignment. This code exists
     * elsewhere and should be reused and the results passed into this API instead.
     */
    void set_align(std::optional<Geom::Point> align = {}) { _align = align; }

    /**
     * This used to be the enum SP_ASPECT_SLICE, but this can just be a bool.
     */
    void set_clip(bool is_slice) { aspect_is_slice = is_slice; }

    /**
     * Set the render's target item box.
     */
    void set_item_box(Geom::OptRect const &item_box) { _item_box = item_box; }

    /**
     * Set the render function which will populate the image surface correctly
     */
    void set_render_function(ImageRenderFunction rf) { _render_function = rf; }

    Glib::ustring name() const override { return Glib::ustring("Image"); }

private:
    ImageRenderFunction _render_function;
    Geom::OptRect _item_box;
    bool from_element = false;

    std::optional<Geom::Point> _align;
    bool aspect_is_slice;
};

} // namespace DrawingFilter
} // namespace Inkscape::Renderer

#endif // SEEN_INKSCAPE_RENDERER_DRAWING_FILTER_IMAGE_H
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
