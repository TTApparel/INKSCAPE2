// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * feMerge filter effect renderer
 *
 * Authors:
 *   Niko Kiirala <niko@kiirala.com>
 *
 * Copyright (C) 2007 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_INKSCAPE_RENDERER_DRAWING_FILTER_MERGE_H
#define SEEN_INKSCAPE_RENDERER_DRAWING_FILTER_MERGE_H

#include <vector>
#include "renderer/drawing-filters/primitive.h"

namespace Inkscape::Renderer::DrawingFilter {

class Merge : public Primitive
{
public:
    Merge();

    void render(Slot &) const override;
    bool can_handle_affine(Geom::Affine const &) const override;
    double complexity(Geom::Affine const &ctm) const override;

    void set_input(int input) override;
    void set_input(int input, int slot) override;

    Glib::ustring name() const override { return Glib::ustring("Merge"); }

    bool uses_input(int slot) const override
    {
        for (int input : _input_image) {
            if (input == slot) {
                return true;
            }
        }
        return false;
    } 

private:
    std::vector<int> _input_image;
};

} // namespace Inkscape::Renderer::DrawingFilter

#endif // SEEN_INKSCAPE_RENDERER_DRAWING_FILTER_MERGE_H
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
