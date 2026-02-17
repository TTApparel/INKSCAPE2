// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * feColorMatrix filter primitive renderer
 *
 * Authors:
 *   Felipe Corrêa da Silva Sanches <juca@members.fsf.org>
 *   Jasper van de Gronde <th.v.d.gronde@hccnet.nl>
 *
 * Copyright (C) 2007 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "color-matrix.h"
#include "colors/manager.h"
#include "slot.h"

#include "renderer/context.h"
#include "renderer/pixel-filters/color-matrix.h"
#include "renderer/surface.h"

namespace Inkscape::Renderer::DrawingFilter {

void ColorMatrix::render(Slot &slot) const
{
    auto input = slot.get(_input, _color_space);

    static auto alpha = Colors::Manager::get().find(Colors::Space::Type::Alpha);
    auto output = input->similar({}, type == ColorMatrixType::LUMINANCETOALPHA ? alpha : input->getColorSpace());

    switch (type) {
    case ColorMatrixType::MATRIX:
        output->run_pixel_filter(PixelFilter::ColorMatrix(values, value), *input);
        break;
    case ColorMatrixType::SATURATE:
        output->run_pixel_filter(PixelFilter::ColorMatrixSaturate(value), *input);
        break;
    case ColorMatrixType::HUEROTATE:
        output->run_pixel_filter(PixelFilter::ColorMatrixHueRotate(value), *input);
        break;
    case ColorMatrixType::LUMINANCETOALPHA:
        output->run_pixel_filter(PixelFilter::ColorMatrixLuminance(), *input);
        break;
    case ColorMatrixType::ENDTYPE:
    default:
        break;
    }

    slot.set(_output, output);
}

bool ColorMatrix::can_handle_affine(Geom::Affine const &) const
{
    return true;
}

double ColorMatrix::complexity(Geom::Affine const &) const
{
    return 2.0;
}

void ColorMatrix::set_type(ColorMatrixType t)
{
    type = t;
}

void ColorMatrix::set_value(double v)
{
    value = v;
}

void ColorMatrix::set_values(std::vector<double> const &v)
{
    values = v;
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
