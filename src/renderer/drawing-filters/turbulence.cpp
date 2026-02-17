// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * feTurbulence filter primitive renderer
 *
 * Authors:
 *   World Wide Web Consortium <http://www.w3.org/>
 *   Felipe Corrêa da Silva Sanches <juca@members.fsf.org>
 *
 * This file has a considerable amount of code adapted from
 *  the W3C SVG filter specs, available at:
 *  http://www.w3.org/TR/SVG11/filters.html#feTurbulence
 *
 * W3C original code is licensed under the terms of
 *  the (GPL compatible) W3C® SOFTWARE NOTICE AND LICENSE:
 *  http://www.w3.org/Consortium/Legal/2002/copyright-software-20021231
 *
 * Copyright (C) 2007 authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "turbulence.h"
#include "slot.h"

#include "renderer/pixel-filters/turbulence.h"
#include "renderer/surface.h"

namespace Inkscape::Renderer::DrawingFilter {

Turbulence::Turbulence()
    : XbaseFrequency(0)
    , YbaseFrequency(0)
    , numOctaves(1)
    , seed(0)
    , updated(false)
    , fTileWidth(10)
    , fTileHeight(10)
    , fTileX(1)
    , fTileY(1)
{}

void Turbulence::set_baseFrequency(int axis, double freq)
{
    if (axis == 0) XbaseFrequency = freq;
    if (axis == 1) YbaseFrequency = freq;
}

void Turbulence::set_numOctaves(int num)
{
    numOctaves = num;
}

void Turbulence::set_seed(double s)
{
    seed = s;
}

void Turbulence::set_stitchTiles(bool st)
{
    stitchTiles = st;
}

void Turbulence::set_type(TurbulenceType t)
{
    type = t;
}

void Turbulence::set_updated(bool /*u*/)
{
}

void Turbulence::render(Slot &slot) const
{
    auto input = slot.get(_input, _color_space);
    if (!input) {
        g_warning("Turbulence filter requires an input for dimensions, ignoring.");
        return;
    }

    auto dest = input->similar();

    Geom::Point ta(fTileX, fTileY);
    Geom::Point tb(fTileX + fTileWidth, fTileY + fTileHeight);

    PixelFilter::Turbulence turbGen(seed, Geom::Rect(ta, tb),
        Geom::Point(XbaseFrequency, YbaseFrequency), stitchTiles,
        type == TurbulenceType::FRACTALNOISE, numOctaves);

    Geom::Affine unit_trans = slot.get_item_options().get_matrix_primitiveunits2pb().inverse();
    turbGen.setAffine(unit_trans);

    auto slot_area = slot.get_item_options().get_slot_box();
    if (slot_area) {
        turbGen.setOrigin(slot_area->min().round());
    }

    turbGen.init();
    dest->run_pixel_filter(turbGen);

    slot.set(_output, dest);
}

double Turbulence::complexity(Geom::Affine const &) const
{
    return 5.0;
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
