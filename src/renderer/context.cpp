// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Cairo drawing context with Inkscape extensions.
 *//*
 * Authors:
 *   Krzysztof Kosiński <tweenk.pl@gmail.com>
 *
 * Copyright (C) 2011 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "context.h"
#include "context-paths.h"

#include "colors/color.h"
#include "colors/spaces/base.h"

#include "surface.h"

namespace Inkscape::Renderer {

/**
 * Create a context with a saved state, restores automatically on destruction.
 */
Context::Context(Context const &parent)
    : _cts(parent._cts)
    , _logical_bounds(parent._logical_bounds)
    , _color_space(parent._color_space)
    , _format(parent._format)
{
    save(); // Mirrors ~Context restore
}

Context::Context(Surface &surface, Geom::OptRect logical_bounds, Geom::Scale const &logical_scale)
    : _logical_bounds(logical_bounds)
    , _color_space(surface.getColorSpace())
    , _format(surface.format())
{
    for (auto cs : surface.getCairoSurfaces()) {
        _cts.emplace_back(Cairo::Context::create(cs));
        _cts.back()->save();
        // Allow scale before origin translation
        if (logical_scale != Geom::identity()) {
            scale(logical_scale);
        }
        if (logical_bounds) {
            translate(Geom::Translate(-logical_bounds->min()));
        }
    }
}

Context::~Context()
{
    restore();
    flush();
}

void Context::arc(Geom::Point const &center, double radius, Geom::AngleInterval const &angle)
{
    double from = angle.initialAngle();
    double to = angle.finalAngle();
    for (auto ct : _cts) {
        if (to > from) {
            ct->arc(center[Geom::X], center[Geom::Y], radius, from, to);
        } else {
            ct->arc_negative(center[Geom::X], center[Geom::Y], radius, to, from);
        }
    }
}

void Context::transform(Geom::Affine const &m) {
    for (auto ct : _cts) {
        ct->transform(geom_to_cairo(m));
    }
}

void Context::path(Geom::PathVector const &pv) {
    for (auto ct : _cts) {
        feed_pathvector_to_cairo(ct, pv);
    }
}

void Context::paint(double alpha) {
    for (auto ct : _cts) {
        if (alpha == 1.0) ct->paint();
        else ct->paint_with_alpha(alpha);
    }
}

void Context::mask(Surface const &surface)
{
    auto &cairo_surfaces = surface.getCairoSurfaces();
    for (unsigned i = 0; i < _cts.size(); i++) {
        _cts[i]->mask(cairo_surfaces[0], 0, 0);
    }
}

void Context::setHairline() {
    for (auto ct : _cts) {
#if CAIRO_VERSION >= CAIRO_VERSION_ENCODE(1, 17, 6)
        cairo_set_hairline(ct->cobj(), true);
#else       
        // As a backup, use a device unit of 1
        auto coord = device_to_user_distance({1, 1});
        ct->set_line_width(coord.length());
#endif  
    }
}

/*
int Context::getDeviceScale() const
{
    return cairomm_114_get_device_scale(cairo_get_group_target(_cts[0]));
}
*/

void Context::setOperator(SPBlendMode op)
{
    static auto get_op = [](SPBlendMode op)
    {
        // Must return cairo enums because cariomm is missing them.
        switch (op) {
        case SP_CSS_BLEND_MULTIPLY:
            return CAIRO_OPERATOR_MULTIPLY;
        case SP_CSS_BLEND_SCREEN:
            return CAIRO_OPERATOR_SCREEN;
        case SP_CSS_BLEND_DARKEN:
            return CAIRO_OPERATOR_DARKEN;
        case SP_CSS_BLEND_LIGHTEN:
            return CAIRO_OPERATOR_LIGHTEN;
        // New in CSS Compositing and Blending Level 1
        case SP_CSS_BLEND_OVERLAY:
            return CAIRO_OPERATOR_OVERLAY;
        case SP_CSS_BLEND_COLORDODGE:
            return CAIRO_OPERATOR_COLOR_DODGE;
        case SP_CSS_BLEND_COLORBURN:
            return CAIRO_OPERATOR_COLOR_BURN;
        case SP_CSS_BLEND_HARDLIGHT:
            return CAIRO_OPERATOR_HARD_LIGHT;
        case SP_CSS_BLEND_SOFTLIGHT:
            return CAIRO_OPERATOR_SOFT_LIGHT;
        case SP_CSS_BLEND_DIFFERENCE:
            return CAIRO_OPERATOR_DIFFERENCE;
        case SP_CSS_BLEND_EXCLUSION:
            return CAIRO_OPERATOR_EXCLUSION;
        case SP_CSS_BLEND_HUE:
            return CAIRO_OPERATOR_HSL_HUE;
        case SP_CSS_BLEND_SATURATION:
            return CAIRO_OPERATOR_HSL_SATURATION;
        case SP_CSS_BLEND_COLOR:
            return CAIRO_OPERATOR_HSL_COLOR;
        case SP_CSS_BLEND_LUMINOSITY:
            return CAIRO_OPERATOR_HSL_LUMINOSITY;
        case SP_CSS_BLEND_NORMAL:
        default:
            return CAIRO_OPERATOR_OVER;
        }
    };

    for (auto &ct : _cts) {
        ct->set_operator((Cairo::Context::Operator)get_op(op));
    }
}

void Context::setSource(Colors::Color const &color) {
    auto c = color.converted(_color_space)->getValues();
    auto a = 0.0;
    std::swap(a, c.back());
    c.resize(_cts.size() * 3);
    int i = 0;
    for (auto ct : _cts) {
        auto r = c[i++];
        auto g = c[i++];
        auto b = c[i++];
        ct->set_source_rgba(r, g, b, a);
    }
}

void Context::setSource(Surface const &surface, double x, double y,
                        std::optional<Cairo::SurfacePattern::Filter> filter,
                        std::optional<Cairo::Pattern::Extend> extend)
{
    auto &cairo_surfaces = surface.getCairoSurfaces();

    // We're going to forbid data mixing in this layer; see PixelFilters instead.
    if (cairo_surfaces.size() != _cts.size() || surface.getColorSpace() != _color_space) {
        std::cerr << "Trying to paint a "
                  << (surface.getColorSpace() ? surface.getColorSpace()->getName() : "{INTRGB}")
                  << " surface into a "
                  << (_color_space ? _color_space->getName() : "{INTRGB}")
                  << " context.\n";
        assert(false);
    }

    // This is the only use of format, to make sure painting operations happen
    if (surface.format() != _format) {
        std::cerr << "Trying to paint two differently formatted surfaces!\n";
        assert(false);
    }
    
    for (unsigned i = 0; i < _cts.size(); i++) {
        _cts[i]->set_source(cairo_surfaces[i], x, y);
        // Cairo converts Surfaces to SurfacePatterns internally.
        auto pattern = _cts[i]->get_source_for_surface();
        if (filter) {
            pattern->set_filter(*filter);
        }
        if (extend) {
            pattern->set_extend(*extend);
        }
    }
}

void Context::setSource(std::unique_ptr<Pattern> const &pattern)
{
    auto &cairo_patterns = pattern->getCairoPatterns();
    if (cairo_patterns.size() != _cts.size() || pattern->getColorSpace() != _color_space) {
        std::cerr << "Surfaces are not compatible! Skipping painting operation\n";
        assert(false);
        return;
    }
    for (unsigned i = 0; i < _cts.size(); i++) {
        _cts[i]->set_source(cairo_patterns[i]);
    }
}

void Context::setAntialias(Antialiasing antialias)
{
    for (auto &ct : _cts) {
        switch (antialias) {
            case Antialiasing::None:
                ct->set_antialias(Cairo::Antialias::ANTIALIAS_NONE);
                break;
            case Antialiasing::Fast:
                // Unavailable in Cairomm 1.16
                ct->set_antialias((Cairo::Antialias)CAIRO_ANTIALIAS_FAST);
                break;
            case Antialiasing::Good:
                ct->set_antialias((Cairo::Antialias)CAIRO_ANTIALIAS_GOOD);
                break;
            case Antialiasing::Best:
                ct->set_antialias((Cairo::Antialias)CAIRO_ANTIALIAS_BEST);
            break;
            default:
                g_assert_not_reached();
        }
    }
}

} // end namespace Inkscape

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
