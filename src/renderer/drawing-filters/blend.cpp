// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * SVG feBlend renderer
 *//*
 * "This filter composites two objects together using commonly used
 * imaging software blending modes. It performs a pixel-wise combination
 * of two input images."
 * http://www.w3.org/TR/SVG11/filters.html#feBlend
 *
 * Authors:
 *   Niko Kiirala <niko@kiirala.com>
 *   Jasper van de Gronde <th.v.d.gronde@hccnet.nl>
 *   Krzysztof Kosiński <tweenk.pl@gmail.com>
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2007-2026 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "blend.h"
#include "slot.h"

#include "renderer/context.h"
#include "renderer/surface.h"

namespace Inkscape::Renderer::DrawingFilter {

Blend::Blend()
    : _blend_mode(SP_CSS_BLEND_NORMAL)
    , _input2(SLOT_NOT_SET)
{}

void Blend::render(Slot &slot) const
{
    auto input1 = slot.get(_input, _color_space);
    auto input2 = slot.get_copy(_input2, _color_space);

    if (input1 && input2) {
        auto context = Context(*input2);
        context.setSource(*input1);
        // All of the blend modes are implemented in Cairo as of 1.10.
        // For a detailed description, see:
        // http://cairographics.org/operators/
        context.setOperator(_blend_mode);
        context.paint();
    }
    slot.set(_output, input2);
}

bool Blend::can_handle_affine(Geom::Affine const &) const
{
    // blend is a per-pixel primitive and is immutable under transformations
    return true;
}

double Blend::complexity(Geom::Affine const &) const
{
    return 1.1;
}

void Blend::set_input(int slot)
{
    _input = slot;
}

void Blend::set_input(int input, int slot)
{
    if (input == 0) _input = slot;
    if (input == 1) _input2 = slot;
}

void Blend::set_mode(SPBlendMode mode)
{
    _blend_mode = mode;
}

} /* namespace Inkscape::Renderer::DrawingFilter */

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
