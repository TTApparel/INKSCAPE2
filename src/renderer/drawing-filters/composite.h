// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * feComposite filter effect renderer
 *
 * Authors:
 *   Niko Kiirala <niko@kiirala.com>
 *
 * Copyright (C) 2007 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_INKSCAPE_RENDERER_DRAWING_FILTER_COMPOSITE_H
#define SEEN_INKSCAPE_RENDERER_DRAWING_FILTER_COMPOSITE_H

#include "primitive.h"

namespace Inkscape::Renderer::DrawingFilter {

class Composite : public Primitive
{
public:
    Composite() = default;
    ~Composite() override = default;

    void render(Slot &) const override;
    bool can_handle_affine(Geom::Affine const &) const override;
    double complexity(Geom::Affine const &ctm) const override;

    void set_input(int input) override;
    void set_input(int input, int slot) override;

    void set_operator(CompositeOperator op);
    void set_arithmetic(double k1, double k2, double k3, double k4);

    Glib::ustring name() const override { return Glib::ustring("Composite"); }

    bool uses_input(int slot)  const override { return _input2 == slot || Primitive::uses_input(slot); }

private:
    CompositeOperator op = CompositeOperator::DEFAULT;
    double k1 = 0, k2 = 0, k3 = 0, k4 = 0;
    int _input2 = SLOT_NOT_SET;
};

} // namespace Inkscape::Renderer::DrawingFilter

#endif // SEEN_INKSCAPE_RENDERER_DRAWING_FILTER_COMPOSITE_H
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
