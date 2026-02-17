// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Cairo based drawing context for Renderer::Surfaces
 *//*
 * Authors:
 *   Krzysztof Kosiński <tweenk.pl@gmail.com>
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2011-2025 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_INKSCAPE_RENDERER_CONTEXT_H
#define SEEN_INKSCAPE_RENDERER_CONTEXT_H

#include <memory>
#include <2geom/transforms.h>
#include <2geom/point.h>
#include <2geom/rect.h>
#include <cairomm/context.h>

#include "colors/forward.h"
#include "context-pattern.h"
#include "renderer/enums.h"

namespace Inkscape::Renderer {

class Surface;

/**
 * @class Context
 * Maximal wrapper over Cairo::Context for drawing on multiple surfaces.
 */
class Context
{
public:
    Context(Context const &parent);
    Context(Surface &s, Geom::OptRect view = {}, Geom::Scale const &scale = {});
    ~Context();
    
    void save() { for (auto &ct : _cts) { ct->save(); } }
    void restore() { for (auto &ct : _cts) { ct->restore(); } }
    void flush() { for (auto &ct : _cts) { ct->get_target()->flush(); } }
    void pushGroup() { for (auto &ct : _cts) { ct->push_group(); } }
    void pushAlphaGroup() { for (auto &ct : _cts) { ct->push_group_with_content(Cairo::Content::CONTENT_COLOR_ALPHA); } }
    void popGroupToSource() { for (auto &ct : _cts) { ct->pop_group_to_source(); } }

    void transform(Geom::Affine const &trans);
    void translate(Geom::Translate const &t) { for (auto &ct : _cts) { ct->translate(t[Geom::X], t[Geom::Y]); } }
    void scale(Geom::Scale const &s) { for (auto &ct : _cts) { ct->scale(s[Geom::X], s[Geom::Y]); } }

    void moveTo(Geom::Point const &p) { for (auto &ct : _cts) { ct->move_to(p[Geom::X], p[Geom::Y]); } }
    void lineTo(Geom::Point const &p) { for (auto &ct : _cts) { ct->line_to(p[Geom::X], p[Geom::Y]); } }
    void curveTo(Geom::Point const &p1, Geom::Point const &p2, Geom::Point const &p3) {
        for (auto &ct : _cts) {
            ct->curve_to(p1[Geom::X], p1[Geom::Y], p2[Geom::X], p2[Geom::Y], p3[Geom::X], p3[Geom::Y]);
        }
    }
    void arc(Geom::Point const &center, double radius, Geom::AngleInterval const &angle);
    void closePath() { for (auto &ct : _cts) { ct->close_path(); } }

    template <typename Rect>
    void rectangle(Rect const &r) {
        for (auto &ct : _cts) {
            ct->rectangle(r.left(), r.top(), r.width(), r.height());
        }
    }
    // Used in drawing-text.cpp to overwrite glyphs, which have the opposite path rotation as a regular rect
    template <typename Rect>
    void reversedRectangle(Rect const &r) {
        for(auto &ct : _cts) {
            ct->move_to(r.left(), r.top());
            ct->rel_line_to(0, r.height());
            ct->rel_line_to(r.width(), 0);
            ct->rel_line_to(0, -r.height());
            ct->close_path();
        }
    }
    void newPath() { for (auto &ct : _cts) { ct->begin_new_path(); } }
    void newSubpath() { for (auto &ct : _cts) { ct->begin_new_sub_path(); } }
    void path(Geom::PathVector const &pv);

    void paint(double alpha = 1.0);
    void mask(Surface const &surface);
    void fill() { for (auto &ct : _cts) { ct->fill(); } }
    void fillPreserve() { for (auto &ct : _cts) { ct->fill_preserve(); } }
    void stroke() { for (auto &ct : _cts) { ct->stroke(); } }
    void strokePreserve() { for (auto &ct : _cts) { ct->stroke_preserve(); } }
    void clip() { for (auto &ct : _cts) { ct->clip(); } }

    void setLineWidth(double w) { for (auto &ct : _cts) { ct->set_line_width(w); } }
    void setHairline();
    void setLineCap(Cairo::Context::LineCap cap) { for (auto &ct : _cts) { ct->set_line_cap(cap); } }
    void setLineJoin(Cairo::Context::LineJoin join) { for (auto &ct : _cts) { ct->set_line_join(join); } }
    void setMiterLimit(double miter) { for (auto &ct : _cts) { ct->set_miter_limit(miter); } }
    void setDash(std::valarray<double> dashes, double offset) {
        for (auto &ct : _cts) {
            ct->set_dash(dashes, offset);
        }
    }
    void setFillRule(Cairo::Context::FillRule rule) { for (auto &ct : _cts) { ct->set_fill_rule(rule); } }
    void setOperator(Cairo::Context::Operator op) { for (auto &ct : _cts) { ct->set_operator(op); } }
    void setOperator(SPBlendMode op);
    Cairo::Context::Operator getOperator() { return _cts[0]->get_operator(); }
    void setAntialias(Antialiasing antialias);
    void setAntialiasing(Cairo::Antialias antialias) { for (auto &ct : _cts) { ct->set_antialias(antialias); } }
    Cairo::Antialias getAntialiasing() const { return _cts[0]->get_antialias(); }
    void setTolerance(double tol) { for(auto &ct : _cts) { ct->set_tolerance(tol); } }
    double getTolerance() const { return _cts[0]->get_tolerance(); }

    void setSource(Colors::Color const &color);
    void setSource(Surface const &surface, double x = 0, double y = 0,
                   std::optional<Cairo::SurfacePattern::Filter> filter = {},
                   std::optional<Cairo::Pattern::Extend> extend = {});
    void setSource(std::unique_ptr<Pattern> const &pattern);
    void resetSource(double a = 1.0) { for (auto &ct : _cts) { ct->set_source_rgba(0, 0, 0, a); } }

    Geom::Point user_to_device_distance(Geom::Point const &pt) {
        double x = pt.x(), y = pt.y();
        cairo_user_to_device_distance(_cts[0]->cobj(), &x, &y); // Cairomm 1.16
        return {x, y};
    }
    Geom::Point device_to_user_distance(Geom::Point const &pt) {
        double x = pt.x(), y = pt.y();
        cairo_device_to_user_distance(_cts[0]->cobj(), &x, &y);
        return {x, y};
    }

    Geom::OptRect logicalBounds() const { return _logical_bounds; }
private:

    std::vector<Cairo::RefPtr<Cairo::Context>> _cts;
    Geom::OptRect _logical_bounds; // origin + dimensions

    std::shared_ptr<Colors::Space::AnySpace> _color_space;
    cairo_format_t _format;
};

} // end namespace Inkscape::Renderer

#endif // !SEEN_INKSCAPE_RENDERER_CONTEXT_H

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
