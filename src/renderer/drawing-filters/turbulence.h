// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * feTurbulence filter primitive renderer
 *
 * Authors:
 *   World Wide Web Consortium <http://www.w3.org/>
 *   Felipe Corrêa da Silva Sanches <juca@members.fsf.org>
 *   Niko Kiirala <niko@kiirala.com>
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

#ifndef SEEN_INKSCAPE_RENDERER_DRAWING_FILTER_TURBULENCE_H
#define SEEN_INKSCAPE_RENDERER_DRAWING_FILTER_TURBULENCE_H

#include <memory>
#include <2geom/forward.h>

#include "primitive.h"

namespace Inkscape::Renderer::DrawingFilter {

enum class TurbulenceType
{
    FRACTALNOISE,
    TURBULENCE,
    ENDTYPE
};

class Turbulence : public Primitive
{
public:
    Turbulence();
    ~Turbulence() override = default;

    void render(Slot &slot) const override;
    double complexity(Geom::Affine const &ctm) const override;

    void set_baseFrequency(int axis, double freq);
    void set_numOctaves(int num);
    void set_seed(double s);
    void set_stitchTiles(bool st);
    void set_type(TurbulenceType t);
    void set_updated(bool u);

    Glib::ustring name() const override { return Glib::ustring("Turbulence"); }

    bool uses_input(int slot) const override { return false; }

private:
    void turbulenceInit(long seed);

    double XbaseFrequency, YbaseFrequency;
    int numOctaves;
    double seed;
    bool stitchTiles;
    TurbulenceType type;
    bool updated;

    double fTileWidth;
    double fTileHeight;

    double fTileX;
    double fTileY;
};

} // namespace Inkscape::Renderer::DrawingFilter

#endif // SEEN_INKSCAPE_RENDERER_DRAWING_FILTER_TURBULENCE_H
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
