// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Draw paths with cairo contexts.
 *//*
 * Authors:
 *   Krzysztof Kosiński <tweenk.pl@gmail.com>
 *   Liam P. White <inkscapebrony@gmail.com>
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2010-2026 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "context-paths.h"

#include <2geom/path.h>
#include <2geom/point.h>
#include <2geom/pathvector.h>
#include <2geom/path-sink.h>
#include <2geom/sbasis-to-bezier.h>

#include "util/scope_exit.h"

namespace Inkscape::Renderer {

Cairo::Matrix geom_to_cairo(const Geom::Affine &affine)
{
    return Cairo::Matrix(affine[0], affine[1], affine[2], affine[3], affine[4], affine[5]);
}

/*
 * Can be called recursively.
 * If optimize_stroke == false, the view Rect is not used.
 */
static void
feed_curve_to_cairo(Cairo::RefPtr<Cairo::Context> ct, Geom::Curve const &c, Geom::Affine const &trans, Geom::Rect const &view, bool optimize_stroke)
{
    using Geom::X;
    using Geom::Y;

    unsigned order = 0;
    if (auto b = dynamic_cast<Geom::BezierCurve const*>(&c)) {
        order = b->order();
    }

    // handle the three typical curve cases
    switch (order) {
    case 1:
    {
        Geom::Point end_tr = c.finalPoint() * trans;
        if (!optimize_stroke) {
            ct->line_to(end_tr[0], end_tr[1]);
        } else {
            Geom::Rect swept(c.initialPoint()*trans, end_tr);
            if (swept.intersects(view)) {
                ct->line_to(end_tr[0], end_tr[1]);
            } else {
                ct->move_to(end_tr[0], end_tr[1]);
            }
        }
    }
    break;
    case 2:
    {
        auto quadratic_bezier = static_cast<Geom::QuadraticBezier const*>(&c);
        std::array<Geom::Point, 3> points;
        for (int i = 0; i < 3; i++) {
            points[i] = quadratic_bezier->controlPoint(i) * trans;
        }
        // degree-elevate to cubic Bezier, since Cairo doesn't do quadratic Beziers
        Geom::Point b1 = points[0] + (2./3) * (points[1] - points[0]);
        Geom::Point b2 = b1 + (1./3) * (points[2] - points[0]);
        if (!optimize_stroke) {
            ct->curve_to(b1[X], b1[Y], b2[X], b2[Y], points[2][X], points[2][Y]);
        } else {
            Geom::Rect swept(points[0], points[2]);
            swept.expandTo(points[1]);
            if (swept.intersects(view)) {
                ct->curve_to(b1[X], b1[Y], b2[X], b2[Y], points[2][X], points[2][Y]);
            } else {
                ct->move_to(points[2][X], points[2][Y]);
            }
        }
    }
    break;
    case 3:
    {
        auto cubic_bezier = static_cast<Geom::CubicBezier const*>(&c);
        std::array<Geom::Point, 4> points;
        for (int i = 0; i < 4; i++) {
            points[i] = cubic_bezier->controlPoint(i);
        }
        //points[0] *= trans; // don't do this one here for fun: it is only needed for optimized strokes
        points[1] *= trans;
        points[2] *= trans;
        points[3] *= trans;
        if (!optimize_stroke) {
            ct->curve_to(points[1][X], points[1][Y], points[2][X], points[2][Y], points[3][X], points[3][Y]);
        } else {
            points[0] *= trans;  // didn't transform this point yet
            Geom::Rect swept(points[0], points[3]);
            swept.expandTo(points[1]);
            swept.expandTo(points[2]);
            if (swept.intersects(view)) {
                ct->curve_to(points[1][X], points[1][Y], points[2][X], points[2][Y], points[3][X], points[3][Y]);
            } else {
                ct->move_to(points[3][X], points[3][Y]);
            }
        }
    }
    break;
    default:
    {
        if (Geom::EllipticalArc const *arc = dynamic_cast<Geom::EllipticalArc const*>(&c)) {
            if (arc->isChord()) {
                Geom::Point endPoint(arc->finalPoint());
                ct->line_to(endPoint[0], endPoint[1]);
            } else {
                Geom::Affine xform = arc->unitCircleTransform() * trans;
                // Don't draw anything if the angle is borked
                if(std::isnan(arc->initialAngle()) || std::isnan(arc->finalAngle())) {
                    std::cerr << "Bad angle while drawing EllipticalArc\n";
                    break;
                }

                // Apply the transformation to the current context
                auto cm = geom_to_cairo(xform);

                ct->save();
                ct->transform(cm);

                // Draw the circle
                if (arc->sweep()) {
                    ct->arc(0, 0, 1, arc->initialAngle(), arc->finalAngle());
                } else {
                    ct->arc_negative(0, 0, 1, arc->initialAngle(), arc->finalAngle());
                }
                // Revert the current context
                ct->restore();
            }
        } else {
            // handles sbasis as well as all other curve types
            // this is very slow
            Geom::Path sbasis_path = Geom::cubicbezierpath_from_sbasis(c.toSBasis(), 0.1);

            // recurse to convert the new path resulting from the sbasis to svgd
            for (const auto & iter : sbasis_path) {
                feed_curve_to_cairo(ct, iter, trans, view, optimize_stroke);
            }
        }
    }
    break;
    }
}


/** Feeds path-creating calls to the cairo context translating them from the Path */
static void
feed_path_to_cairo(Cairo::RefPtr<Cairo::Context> ct, Geom::Path const &path)
{
    if (path.empty())
        return;

    ct->move_to(path.initialPoint()[0], path.initialPoint()[1] );

    for (Geom::Path::const_iterator cit = path.begin(); cit != path.end_open(); ++cit) {
        feed_curve_to_cairo(ct, *cit, Geom::identity(), Geom::Rect(), false); // optimize_stroke is false, so the view rect is not used
    }

    if (path.closed()) {
        ct->close_path();
    }
}

/** Feeds path-creating calls to the cairo context translating them from the Path, with the given transform and shift */
static void
feed_path_to_cairo(Cairo::RefPtr<Cairo::Context> ct, Geom::Path const &path, Geom::Affine trans, Geom::OptRect area, bool optimize_stroke, double stroke_width)
{
    if (!area)
        return;
    if (path.empty())
        return;

    // Transform all coordinates to coords within "area"
    Geom::Point shift = area->min();
    Geom::Rect view = *area;
    view.expandBy (stroke_width);
    view = view * (Geom::Affine)Geom::Translate(-shift);
    //  Pass transformation to feed_curve, so that we don't need to create a whole new path.
    Geom::Affine transshift(trans * Geom::Translate(-shift));

    Geom::Point initial = path.initialPoint() * transshift;
    ct->move_to(initial[0], initial[1] );

    for(Geom::Path::const_iterator cit = path.begin(); cit != path.end_open(); ++cit) {
        feed_curve_to_cairo(ct, *cit, transshift, view, optimize_stroke);
    }

    if (path.closed()) {
        if (!optimize_stroke) {
            ct->close_path();
        } else {
            ct->line_to(initial[0], initial[1]);
            /* We cannot use cairo_close_path(ct) here because some parts of the path may have been
               clipped and not drawn (maybe the before last segment was outside view area), which 
               would result in closing the "subpath" after the last interruption, not the entire path.

               However, according to cairo documentation:
               The behavior of cairo_close_path() is distinct from simply calling cairo_line_to() with the equivalent coordinate
               in the case of stroking. When a closed sub-path is stroked, there are no caps on the ends of the sub-path. Instead,
               there is a line join connecting the final and initial segments of the sub-path. 

               The correct fix will be possible when cairo introduces methods for moving without
               ending/starting subpaths, which we will use for skipping invisible segments; then we
               will be able to use cairo_close_path here. This issue also affects ps/eps/pdf export,
               see bug 168129
            */
        }
    }
}

/** Feeds path-creating calls to the cairo context translating them from the PathVector, with the given transform and shift
 *  One must have done cairo_new_path(ct); before calling this function. */
void feed_pathvector_to_cairo(Cairo::RefPtr<Cairo::Context> ct, Geom::PathVector const &pathv, Geom::Affine trans, Geom::OptRect area, bool optimize_stroke, double stroke_width)
{
    if (!area)
        return;
    if (pathv.empty())
        return;

    for(const auto & it : pathv) {
        feed_path_to_cairo(ct, it, trans, area, optimize_stroke, stroke_width);
    }
}

/** Feeds path-creating calls to the cairo context translating them from the PathVector
 *  One must have done cairo_new_path(ct); before calling this function. */
void feed_pathvector_to_cairo(Cairo::RefPtr<Cairo::Context> ct, Geom::PathVector const &pathv)
{
    if (pathv.empty())
        return;

    for(const auto & it : pathv) {
        feed_path_to_cairo(ct, it);
    }
}

/*
 * Pulls out the last cairo path context and reconstitutes it
 * into a local geom path vector for inkscape use.
 *
 * @param ct - The cairo context
 *
 * @returns an optioal Geom::PathVector object
 */
std::optional<Geom::PathVector> extract_pathvector_from_cairo(cairo_t *ct)
{
    cairo_path_t *path = cairo_copy_path(ct);
    if (!path)
        return std::nullopt;

    auto path_freer = scope_exit([&] { cairo_path_destroy(path); });

    Geom::PathBuilder res;
    auto end = &path->data[path->num_data];
    for (auto p = &path->data[0]; p < end; p += p->header.length) {
        switch (p->header.type) {
            case CAIRO_PATH_MOVE_TO:
                if (p->header.length != 2)
                    return std::nullopt;
                res.moveTo(Geom::Point(p[1].point.x, p[1].point.y));
                break;

            case CAIRO_PATH_LINE_TO:
                if (p->header.length != 2)
                    return std::nullopt;
                res.lineTo(Geom::Point(p[1].point.x, p[1].point.y));
                break;

            case CAIRO_PATH_CURVE_TO:
                if (p->header.length != 4)
                    return std::nullopt;
                res.curveTo(Geom::Point(p[1].point.x, p[1].point.y), Geom::Point(p[2].point.x, p[2].point.y),
                            Geom::Point(p[3].point.x, p[3].point.y));
                break;

            case CAIRO_PATH_CLOSE_PATH:
                if (p->header.length != 1)
                    return std::nullopt;
                res.closePath();
                break;
            default:
                return std::nullopt;
        }
    }

    res.flush();
    return res.peek();
}

}

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
