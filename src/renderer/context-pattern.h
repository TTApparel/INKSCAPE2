// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Drawing patterns in cairo
 *//*
 * Copyright (C) 2026 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_INKSCAPE_RENDERER_CONTEXT_PATTERN_H
#define SEEN_INKSCAPE_RENDERER_CONTEXT_PATTERN_H

#include <memory>
#include <2geom/forward.h>
#include <cairomm/pattern.h>

#include "colors/forward.h"

namespace Inkscape::Renderer {

class Surface;

class Pattern
{
public:
    Pattern(Surface const &surface);
    Pattern(Colors::Color solid_color);

    auto &getCairoPatterns() const { return _pts; }
    auto getColorSpace() const { return _color_space; }

    void setExtend(Cairo::Pattern::Extend extend) {
        for (auto &pt : _pts) { pt->set_extend(extend); }
    }
private:

    std::vector<Cairo::RefPtr<Cairo::Pattern>> _pts;
    std::shared_ptr<Colors::Space::AnySpace> _color_space;
};

} // end namespace Inkscape::Renderer

#endif // !SEEN_INKSCAPE_RENDERER_CONTEXT_PATTERN_H

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
