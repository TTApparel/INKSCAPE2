// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Representation of paint servers used when rendering.
 */
/*
 * Author: PBS <pbs3141@gmail.com>
 * Copyright (C) 2022 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_RENDERER_DRAWING_PAINT_SERVER_H
#define INKSCAPE_RENDERER_DRAWING_PAINT_SERVER_H

#include <array>
#include <vector>
#include <2geom/rect.h>
#include <2geom/affine.h>
#include "object/sp-gradient-spread.h"
#include "object/sp-gradient-units.h"
#include "object/sp-gradient-vector.h"

#include "colors/forward.h"
class SPGradient;

namespace Inkscape::Renderer {

class Surface;
class Context;
/**
 * A DrawingPaintServer is a lightweight copy of the resources needed to paint using a paint server.
 *
 * It is built by an SPPaintServer, stored in the DrawingItem tree, and used when rendering.
 *
 * The pattern information is stored in a rendering-backend agnostic way, and the object remains
 * valid even after the original SPPaintServer is modified or destroyed.
 */
class DrawingPaintServer
{
public:
    virtual ~DrawingPaintServer() = 0;

    /// Produce a pattern that can be used for painting with Cairo.
    virtual Surface create_pattern(Context *, Geom::OptRect const &bbox, double opacity) const = 0;

    /// Return whether this paint server could benefit from dithering.
    virtual bool ditherable() const { return false; }

    /// Return whether create_pattern() uses its DrawingContext argument. Such pattern cannot be cached, but recreated each time.
    /// Fixme: The only reson this exists is to work around https://gitlab.freedesktop.org/cairo/cairo/-/issues/146.
    virtual bool uses_cairo_ctx() const { return false; }
};

// Todo: Remove, merging with existing implementation for solid colours.
/**
 * A simple solid color, storing an RGB color and an opacity.
 */
class DrawingSolidColor final
    : public DrawingPaintServer
{
public:
    DrawingSolidColor(Colors::Color color);
    Surface create_pattern(Context *, Geom::OptRect const &, double opacity) const override;

private:
    Colors::Color color;
};

/**
 * The base class for all gradients.
 */
class DrawingGradient
    : public DrawingPaintServer
{
protected:
    DrawingGradient(SPGradientSpread spread, SPGradientUnits units, Geom::Affine const &transform)
        : spread(spread)
        , units(units)
        , transform(transform) {}

    bool ditherable() const override { return true; }

    /// Perform some common initialization steps on the given Cairo pattern.
    void common_setup(Surface *pat, Geom::OptRect const &bbox, double opacity) const;

    SPGradientSpread spread;
    SPGradientUnits units;
    Geom::Affine transform;
};

/**
 * A linear gradient.
 */
class DrawingLinearGradient final
    : public DrawingGradient
{
public:
    DrawingLinearGradient(SPGradientSpread spread, SPGradientUnits units, Geom::Affine const &transform,
                          float x1, float y1, float x2, float y2, std::vector<SPGradientStop> stops)
        : DrawingGradient(spread, units, transform)
        , x1(x1)
        , y1(y1)
        , x2(x2)
        , y2(y2)
        , stops(std::move(stops)) {}

    Surface create_pattern(Context*, Geom::OptRect const &bbox, double opacity) const override;

private:
    float x1, y1, x2, y2;
    std::vector<SPGradientStop> stops;
};

/**
 * A radial gradient.
 */
class DrawingRadialGradient final
    : public DrawingGradient
{
public:
    DrawingRadialGradient(SPGradientSpread spread, SPGradientUnits units, Geom::Affine const &transform,
                          float fx, float fy, float cx, float cy, float r, float fr, std::vector<SPGradientStop> stops)
        : DrawingGradient(spread, units, transform)
        , fx(fx)
        , fy(fy)
        , cx(cx)
        , cy(cy)
        , r(r)
        , fr(fr)
        , stops(std::move(stops)) {}

    Surface create_pattern(Context *ct, Geom::OptRect const &bbox, double opacity) const override;

    bool uses_cairo_ctx() const override { return true; }

private:
    float fx, fy, cx, cy, r, fr;
    std::vector<SPGradientStop> stops;
};

/**
 * A mesh gradient.
 */
class DrawingMeshGradient final
    : public DrawingGradient
{
public:
    struct PatchData
    {
        Geom::Point points[4][4];
        char pathtype[4];
        bool tensorIsSet[4];
        Geom::Point tensorpoints[4];
        std::optional<Colors::Color> color[4];
    };

    DrawingMeshGradient(SPGradientSpread spread, SPGradientUnits units, Geom::Affine const &transform,
                        int rows, int cols, std::vector<std::vector<PatchData>> patchdata)
        : DrawingGradient(spread, units, transform)
        , rows(rows)
        , cols(cols)
        , patchdata(std::move(patchdata)) {}

    Surface create_pattern(Context*, Geom::OptRect const &bbox, double opacity) const override;

private:
    int rows;
    int cols;
    std::vector<std::vector<PatchData>> patchdata;
};

} // namespace Inkscape::Renderer

#endif // INKSCAPE_RENDERER_DRAWING_PAINT_SERVER_H
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
