// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * feConvolveMatrix filter primitive renderer
 *
 * Authors:
 *   Felipe Corrêa da Silva Sanches <juca@members.fsf.org>
 *
 * Copyright (C) 2007 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_INKSCAPE_RENDERER_DRAWING_FILTER_CONVOLVE_MATRIX_H
#define SEEN_INKSCAPE_RENDERER_DRAWING_FILTER_CONVOLVE_MATRIX_H

#include <vector>

#include "primitive.h"

namespace Inkscape::Renderer::DrawingFilter {

class Slot;

enum class ConvolveMatrixEdgeMode
{
    DUPLICATE,
    WRAP,
    NONE,
    ENDTYPE
};

class ConvolveMatrix : public Primitive
{
public:
    ConvolveMatrix() = default;
    ~ConvolveMatrix() override = default;

    void render(Slot &slot) const override;
    void area_enlarge(Geom::IntRect &area, Geom::Affine const &trans) const override;
    double complexity(Geom::Affine const &ctm) const override;

    void set_targetY(int coord);
    void set_targetX(int coord);
    void set_orderY(int coord);
    void set_orderX(int coord);
    void set_kernelMatrix(std::vector<gdouble> km);
    void set_bias(double b);
    void set_divisor(double d);
    void set_edgeMode(ConvolveMatrixEdgeMode mode);
    void set_preserveAlpha(bool pa);

    Glib::ustring name() const override { return Glib::ustring("Convolve Matrix"); }

private:
    std::vector<double> kernelMatrix;
    int targetX, targetY;
    int orderX, orderY;
    double divisor, bias;
    ConvolveMatrixEdgeMode edgeMode;
    bool preserveAlpha;
};

} // namespace Inkscape::Renderer::DrawingFilter

#endif // SEEN_INKSCAPE_RENDERER_DRAWING_FILTER_CONVOLVE_MATRIX_H
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
