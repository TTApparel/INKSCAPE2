// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * feConvolveMatrix filter primitive renderer
 *
 * Authors:
 *   Felipe Corrêa da Silva Sanches <juca@members.fsf.org>
 *   Jasper van de Gronde <th.v.d.gronde@hccnet.nl>
 *
 * Copyright (C) 2007,2009 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "convolve-matrix.h"
#include "slot.h"

#include "renderer/context.h"
#include "renderer/pixel-filters/convolve-matrix.h"
#include "renderer/surface.h"

namespace Inkscape::Renderer::DrawingFilter {

void ConvolveMatrix::render(Slot &slot) const
{
    if (orderX<=0 || orderY<=0) {
        g_warning("Empty kernel!");
        return;
    }
    if (targetX<0 || targetX>=orderX || targetY<0 || targetY>=orderY) {
        g_warning("Invalid target!");
        return;
    }
    if (kernelMatrix.size()!=(unsigned int)(orderX*orderY)) {
        g_warning("kernelMatrix does not have orderX*orderY elements!");
        return;
    }

    auto input = slot.get(_input, _color_space);
    auto output = slot.get_copy(_input, _color_space);
    switch(edgeMode) {
        case ConvolveMatrixEdgeMode::DUPLICATE:
            output->run_pixel_filter<PixelAccessEdgeMode::EXTEND>(PixelFilter::ConvolveMatrix(targetX, targetY, orderX, orderY, divisor, bias, kernelMatrix, preserveAlpha), *input);
            break;
        case ConvolveMatrixEdgeMode::WRAP:
            output->run_pixel_filter<PixelAccessEdgeMode::WRAP>(PixelFilter::ConvolveMatrix(targetX, targetY, orderX, orderY, divisor, bias, kernelMatrix, preserveAlpha), *input);
            break;
        case ConvolveMatrixEdgeMode::NONE:
        case ConvolveMatrixEdgeMode::ENDTYPE:
        default:
            output->run_pixel_filter<PixelAccessEdgeMode::ZERO>(PixelFilter::ConvolveMatrix(targetX, targetY, orderX, orderY, divisor, bias, kernelMatrix, preserveAlpha), *input);
    }
    slot.set(_output, output);
}

void ConvolveMatrix::set_targetX(int coord)
{
    targetX = coord;
}

void ConvolveMatrix::set_targetY(int coord)
{
    targetY = coord;
}

void ConvolveMatrix::set_orderX(int coord)
{
    orderX = coord;
}

void ConvolveMatrix::set_orderY(int coord)
{
    orderY = coord;
}

void ConvolveMatrix::set_divisor(double d)
{
    divisor = d;
}

void ConvolveMatrix::set_bias(double b)
{
    bias = b;
}

void ConvolveMatrix::set_kernelMatrix(std::vector<gdouble> km)
{
    kernelMatrix = std::move(km);
}

void ConvolveMatrix::set_edgeMode(ConvolveMatrixEdgeMode mode)
{
    edgeMode = mode;
}

void ConvolveMatrix::set_preserveAlpha(bool pa)
{
    preserveAlpha = pa;
}

void ConvolveMatrix::area_enlarge(Geom::IntRect &area, Geom::Affine const &/*trans*/) const
{
    //Seems to me that since this filter's operation is resolution dependent,
    // some spurious pixels may still appear at the borders when low zooming or rotating. Needs a better fix.
    area.setMin(area.min() - Geom::IntPoint(targetX, targetY));
    // This makes sure the last row/column in the original image corresponds
    // to the last row/column in the new image that can be convolved without
    // adjusting the boundary conditions).
    area.setMax(area.max() + Geom::IntPoint(orderX - targetX - 1, orderY - targetY - 1));
}

double ConvolveMatrix::complexity(Geom::Affine const &) const
{
    return kernelMatrix.size();
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
