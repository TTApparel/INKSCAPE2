// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * feDiffuseLighting renderer
 *
 * Copyright (C) 2007-2026 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "renderer/context.h"
#include "renderer/surface.h"
#include "renderer/pixel-filters/light.h"

#include "light.h"
#include "slot.h"
#include "units.h"

#include "colors/manager.h"

namespace Inkscape::Renderer::DrawingFilter {

void DiffuseLighting::render(Slot &slot) const
{
    static auto alpha = Colors::Manager::get().find(Colors::Space::Type::Alpha);
    static auto rgb = Colors::Manager::get().find(Colors::Space::Type::RGB);
    static auto default_color = Colors::Color(rgb, {0, 0, 0, 1});

    // Only alpha channel of input is used, no need to check input color_interpolation_filter value.
    auto input = slot.get_copy(_input, _color_space);
    auto output = slot.get(_input)->similar({}, _color_space);

    int device_scale = slot.get_drawing_options().device_scale;
    Geom::Rect slot_area = *slot.get_item_options().get_slot_box();
    Geom::Point p = slot_area.min();

    // trans has inverse y... so we can't just scale by device_scale! We must instead explicitly
    // scale the point and spot light coordinates (as well as "scale").

    Geom::Affine trans = slot.get_item_options().get_matrix_primitiveunits2pb();

    double x0 = p.x(), y0 = p.y();
    double scale = surfaceScale * trans.descrim() * device_scale;

    auto color = lighting_color.value_or(default_color).converted(_color_space)->getValues();

    switch (light_type) {
    case DISTANT_LIGHT:
        output->run_pixel_filter<PixelAccessEdgeMode::ZERO>(PixelFilter::DistantLight(
            light.distant.azimuth,
            light.distant.elevation,
            color,
            scale,
            diffuseConstant,
            specularExponent
        ), *input);
        break;
    case POINT_LIGHT:
        output->run_pixel_filter<PixelAccessEdgeMode::ZERO>(PixelFilter::PointLight(
            {light.point.x, light.point.y, light.point.x},
            x0, y0,
            trans,
            device_scale,
            color,
            scale,
            diffuseConstant,
            specularExponent
        ), *input);
        break;
    case SPOT_LIGHT:
        output->run_pixel_filter<PixelAccessEdgeMode::ZERO>(PixelFilter::SpotLight(
            {light.spot.x, light.spot.y, light.spot.x},
            {light.spot.pointsAtX, light.spot.pointsAtY, light.spot.pointsAtZ},
            light.spot.limitingConeAngle,
            light.spot.specularExponent,
            x0, y0,
            trans,
            device_scale,
            color,
            scale,
            diffuseConstant,
            specularExponent
        ), *input);
        break;
    default: {
        auto ct = Context(*output);
        ct.paint(); // Fill with black
        break;
        }
    }
    slot.set(_output, output);
}

void DiffuseLighting::area_enlarge(Geom::IntRect &area, Geom::Affine const & /*trans*/) const
{
    // TODO: support kernelUnitLength

    // We expand the area by 1 in every direction to avoid artifacts on tile edges.
    // However, it means that edge pixels will be incorrect.
    area.expandBy(1);
}

double DiffuseLighting::complexity(Geom::Affine const &) const
{
    return 9.0;
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
