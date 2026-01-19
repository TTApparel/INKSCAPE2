// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Drawing patterns in cairo
 *//*
 * Copyright (C) 2026 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "context-pattern.h"
#include "colors/color.h"

#include "surface.h"

namespace Inkscape::Renderer {

Pattern::Pattern(Surface const &surface)
    : _color_space(surface.getColorSpace())
{
    for (auto &s : surface.getCairoSurfaces()) {
        _pts.emplace_back(Cairo::SurfacePattern::create(s));
    }
}

Pattern::Pattern(Colors::Color solid_color)
    : _color_space(solid_color.getSpace())
{
    auto a = 0.0;
    bool has_a = solid_color.hasOpacity();
    auto c = solid_color.getValues();

    if (has_a) {
        // Steal alpha
        std::swap(a, c.back());
    }
    c.resize(c.size() + 3); // Blind pad

    for (unsigned i = 0; i < solid_color.size(); i += 3) {
        if (has_a) {
            _pts.emplace_back(Cairo::SolidPattern::create_rgba(c[i], c[i+1], c[i+2], a));
        } else {
            _pts.emplace_back(Cairo::SolidPattern::create_rgb(c[i], c[i+1], c[i+2]));
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
