// SPDX-License-Identifier: GPL-2.0-or-later
#include "drawing-paintserver.h"

#include <utility>

#include "cairo-utils.h"
#include "colors/color.h"

#include "display/drawing-context.h"

namespace Inkscape {

DrawingPaintServer::~DrawingPaintServer() = default;

DrawingSolidColor::DrawingSolidColor(Colors::Color color)
    : color(std::move(color)) {}

cairo_pattern_t *DrawingSolidColor::create_pattern(DrawingContext *, Geom::OptRect const &, double opacity) const
{
    return ink_cairo_pattern_create(color.withOpacity(opacity));
}

void DrawingGradient::common_setup(cairo_pattern_t *pat, Geom::OptRect const &bbox, double opacity) const
{
    // set spread type
    switch (spread) {
        case SP_GRADIENT_SPREAD_REFLECT:
            cairo_pattern_set_extend(pat, CAIRO_EXTEND_REFLECT);
            break;
        case SP_GRADIENT_SPREAD_REPEAT:
            cairo_pattern_set_extend(pat, CAIRO_EXTEND_REPEAT);
            break;
        case SP_GRADIENT_SPREAD_PAD:
        default:
            cairo_pattern_set_extend(pat, CAIRO_EXTEND_PAD);
            break;
    }

    // set pattern transform matrix
    auto gs2user = transform;
    if (units == SP_GRADIENT_UNITS_OBJECTBOUNDINGBOX && bbox) {
        auto bbox2user = Geom::Affine(bbox->width(), 0, 0, bbox->height(), bbox->left(), bbox->top());
        gs2user *= bbox2user;
    }
    ink_cairo_pattern_set_matrix(pat, gs2user.inverse());
}

cairo_pattern_t *DrawingLinearGradient::create_pattern(DrawingContext *, Geom::OptRect const &bbox, double opacity) const
{
    auto pat = cairo_pattern_create_linear(x1, y1, x2, y2);

    common_setup(pat, bbox, opacity);

    // add stops
    for (auto &stop : stops) {
        // multiply stop opacity by paint opacity
        if (stop.color.has_value()) {
            ink_cairo_pattern_add_color_stop(pat, stop.offset, stop.color->withOpacity(opacity));
        }
    }

    return pat;
}

cairo_pattern_t *DrawingRadialGradient::create_pattern(DrawingContext *ct, Geom::OptRect const &bbox, double opacity) const
{
    Geom::Point focus(fx, fy);
    Geom::Point center(cx, cy);

    double radius = r;
    double focusr = fr;
    double scale = 1.0;
    double tolerance = ct->getTolerance();

    Geom::Affine gs2user = transform;

    if (units == SP_GRADIENT_UNITS_OBJECTBOUNDINGBOX && bbox) {
        Geom::Affine bbox2user(bbox->width(), 0, 0, bbox->height(), bbox->left(), bbox->top());
        gs2user *= bbox2user;
    }

    // we need to use vectors with the same direction to represent the transformed
    // radius and the focus-center delta, because gs2user might contain non-uniform scaling
    Geom::Point d(focus - center);
    Geom::Point d_user(d.length(), 0);
    Geom::Point r_user(radius, 0);
    Geom::Point fr_user(focusr, 0);
    d_user *= gs2user.withoutTranslation();
    r_user *= gs2user.withoutTranslation();
    fr_user *= gs2user.withoutTranslation();

    auto d_device = ct->get_device_distance(d_user);

    // compute the tolerance distance in user space
    // create a vector with the same direction as the transformed d,
    // with the length equal to tolerance
    double dl = d_device.length();
    auto t_device = Geom::Point(tolerance * d_device.x() / dl,
                                tolerance * d_device.y() / dl);
    auto tolerance_user = ct->get_user_distance(t_device).length();

    if (d_user.length() + tolerance_user > r_user.length()) {
        scale = r_user.length() / d_user.length();

        // nudge the focus slightly inside
        scale *= 1.0 - 2.0 * tolerance / dl;
    }

    auto pat = cairo_pattern_create_radial(scale * d.x() + center.x(), scale * d.y() + center.y(), focusr, center.x(), center.y(), radius);

    common_setup(pat, bbox, opacity);

    // add stops
    for (auto &stop : stops) {
        // multiply stop opacity by paint opacity
        if (stop.color.has_value()) {
            ink_cairo_pattern_add_color_stop(pat, stop.offset, stop.color->withOpacity(opacity));
        }
    }

    return pat;
}

cairo_pattern_t *DrawingMeshGradient::create_pattern(DrawingContext *, Geom::OptRect const &bbox, double opacity) const
{
#ifdef MESH_DEBUG
    std::cout << "sp_meshgradient_create_pattern: " << bbox << " " << opacity << std::endl;
#endif

    auto pat = cairo_pattern_create_mesh();

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            auto &data = patchdata[i][j];

            cairo_mesh_pattern_begin_patch(pat);
            cairo_mesh_pattern_move_to(pat, data.points[0][0].x(), data.points[0][0].y());

            for (int k = 0; k < 4; k++) {
                switch (data.pathtype[k]) {
                case 'l':
                case 'L':
                case 'z':
                case 'Z':
                    cairo_mesh_pattern_line_to(pat, data.points[k][3].x(), data.points[k][3].y());
                    break;
                case 'c':
                case 'C':
                    cairo_mesh_pattern_curve_to(pat, data.points[k][1].x(), data.points[k][1].y(),
                            data.points[k][2].x(), data.points[k][2].y(),
                            data.points[k][3].x(), data.points[k][3].y());
                    break;
                default:
                    // Shouldn't happen
                    std::cerr << "sp_mesh_create_pattern: path error" << std::endl;
                }

                if (data.tensorIsSet[k]) {
                    Geom::Point t = data.tensorpoints[k];
                    cairo_mesh_pattern_set_control_point(pat, k, t.x(), t.y());
                }

                if (data.color[k]) {
                    ink_cairo_mesh_pattern_set_corner_color(pat, k, data.color[k]->withOpacity(opacity));
                } else {
                    std::cerr << "Bad mesh color at pos " << k << "\n";
                    static auto const black = *Colors::Color::parse("black");
                    ink_cairo_mesh_pattern_set_corner_color(pat, k, black.withOpacity(opacity));
                }
            }

            cairo_mesh_pattern_end_patch(pat);
        }
    }

    // set pattern transform matrix
    Geom::Affine gs2user = transform;
    if (units == SP_GRADIENT_UNITS_OBJECTBOUNDINGBOX && bbox) {
        Geom::Affine bbox2user(bbox->width(), 0, 0, bbox->height(), bbox->left(), bbox->top());
        gs2user *= bbox2user;
    }
    ink_cairo_pattern_set_matrix(pat, gs2user.inverse());

    return pat;
}

} // namespace Inkscape

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
