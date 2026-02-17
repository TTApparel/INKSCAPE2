// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors: see git history
 *
 * Copyright (C) 2014-2026 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */
#ifndef SEEN_INKSCAPE_RENDERER_DRAWING_FILTER_LIGHT_TYPES_H
#define SEEN_INKSCAPE_RENDERER_DRAWING_FILTER_LIGHT_TYPES_H

namespace Inkscape::Renderer::DrawingFilter {

enum LightType
{
    NO_LIGHT = 0,
    DISTANT_LIGHT,
    POINT_LIGHT,
    SPOT_LIGHT
};

struct DistantLightData
{
    double azimuth, elevation;
};

struct PointLightData
{
    double x, y, z;
};

struct SpotLightData
{
    double x, y, z;
    double pointsAtX, pointsAtY, pointsAtZ;
    double limitingConeAngle;
    double specularExponent;
};

} // namespace Inkscape::Renderer::DrawingFilter

#endif // SEEN_INKSCAPE_RENDERER_DRAWING_FILTER_LIGHT_TYPES_H
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
