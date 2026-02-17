// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * feDiffuseLighting renderer
 *
 * Copyright (C) 2007-2026 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_INKSCAPE_RENDERER_DRAWING_FILTER_LIGHT_DIFFUSE_H
#define SEEN_INKSCAPE_RENDERER_DRAWING_FILTER_LIGHT_DIFFUSE_H

#include <optional>
#include "enums.h"
#include "primitive.h"
#include "colors/color.h"

#include "light-types.h"

namespace Inkscape::Renderer::DrawingFilter {

class DiffuseLighting : public Primitive
{
public:
    DiffuseLighting() = default;
    ~DiffuseLighting() override = default;

    void render(Slot &slot) const override;
    void area_enlarge(Geom::IntRect &area, Geom::Affine const &trans) const override;
    double complexity(Geom::Affine const &ctm) const override;

    union {
        DistantLightData distant;
        PointLightData point;
        SpotLightData spot;
    } light;

    LightType light_type;

    double diffuseConstant;
    double surfaceScale;
    std::optional<Colors::Color> lighting_color;

    // If not set, this will be a diffuse light. If set it will be a specular light.
    std::optional<double> specularExponent;

    Glib::ustring name() const override { return "Diffuse Lighting"; }
};

class SpecularLighting : public DiffuseLighting
{
    Glib::ustring name() const override { return "Specular Lighting"; }
};

} // namespace Inkscape::Renderer::DrawingFilter

#endif // SEEN_INKSCAPE_RENDERER_DRAWING_FILTER_LIGHT_DIFFUSE_H
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
