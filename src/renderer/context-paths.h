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

#ifndef SEEN_INKSCAPE_RENDERER_CONTEXT_PATHS_H
#define SEEN_INKSCAPE_RENDERER_CONTEXT_PATHS_H

#include <2geom/forward.h>
#include <cairomm/context.h>
#include <optional>

namespace Inkscape::Renderer {

Cairo::Matrix geom_to_cairo(const Geom::Affine &affine);

void feed_pathvector_to_cairo(Cairo::RefPtr<Cairo::Context> ct, Geom::PathVector const &pathv, Geom::Affine trans, Geom::OptRect area, bool optimize_stroke, double stroke_width);
void feed_pathvector_to_cairo(Cairo::RefPtr<Cairo::Context> ct, Geom::PathVector const &pathv);

std::optional<Geom::PathVector> extract_pathvector_from_cairo(cairo_t *ct);

} // namespace Inkscape::Renderer

#endif // SEEN_INKSCAPE_RENDERER_CONTEXT_PATHS_H

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
