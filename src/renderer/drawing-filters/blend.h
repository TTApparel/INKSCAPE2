// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * SVG feBlend renderer
 *
 * "This filter composites two objects together using commonly used
 * imaging software blending modes. It performs a pixel-wise combination
 * of two input images."
 * http://www.w3.org/TR/SVG11/filters.html#feBlend
 *
 * Authors:
 *   Niko Kiirala <niko@kiirala.com>
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2007-2026 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_INKSCAPE_RENDERER_DRAWING_FILTER_BLEND_H
#define SEEN_INKSCAPE_RENDERER_DRAWING_FILTER_BLEND_H

#include <set>

#include "style-enums.h"
#include "primitive.h"

namespace Inkscape::Renderer::DrawingFilter {

class Blend : public Primitive
{
public:
    Blend();
    ~Blend() override = default;

    void render(Slot &slot) const override;
    bool can_handle_affine(Geom::Affine const &) const override;
    double complexity(Geom::Affine const &ctm) const override;

    void set_input(int slot) override;
    void set_input(int input, int slot) override;
    void set_mode(SPBlendMode mode);

    Glib::ustring name() const override { return Glib::ustring("Blend"); }

    bool uses_input(int slot) const override
    {
        return _input == slot || _input2 == slot;
    }

private:
    static const std::set<SPBlendMode> _valid_modes;
    SPBlendMode _blend_mode;
    int _input2;
};

} // namespace Inkscape::Renderer::DrawingFilter

#endif // SEEN_INKSCAPE_RENDERER_DRAWING_FILTER_BLEND_H
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
