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

#ifndef SEEN_INKSCAPE_RENDERER_DRAWING_FILTER_MORPHOLOGY_H
#define SEEN_INKSCAPE_RENDERER_DRAWING_FILTER_MORPHOLOGY_H

#include "primitive.h"

namespace Inkscape::Renderer::DrawingFilter {

class FilterSlot;

enum class MorphologyOperator
{
    ERODE,
    DILATE,
    END
};

class Morphology : public Primitive
{
public:
    Morphology() = default;
    ~Morphology() override = default;

    void render(Slot &slot) const override;
    void area_enlarge(Geom::IntRect &area, Geom::Affine const &trans) const override;
    double complexity(Geom::Affine const &ctm) const override;

    void set_operator(MorphologyOperator o);
    void set_xradius(double x);
    void set_yradius(double y);

    Glib::ustring name() const override { return Glib::ustring("Morphology"); }

private:
    MorphologyOperator Operator;
    double xradius;
    double yradius;
};

} // namespace Inkscape::Renderer::DrawingFilter

#endif // SEEN_INKSCAPE_RENDERER_DRAWING_FILTER_MORPHOLOGY_H
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
