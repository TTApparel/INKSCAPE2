// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * DrawingOptions for drawing objects
 *//*
 * Copyright (C) 2026 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_INKSCAPE_RENDERER_DRAWING_ENUMS_H
#define SEEN_INKSCAPE_RENDERER_DRAWING_ENUMS_H

#include "renderer/drawing-filters/enums.h"
#include "renderer/pixel-filters/enums.h"

#include "style-enums.h" // SPBlendMode

namespace Inkscape::Renderer {

enum class Antialiasing : unsigned char
{
    None, Fast, Good, Best
};

} // end namespace Inkscape::Renderer

#endif // !SEEN_INKSCAPE_RENDERER_DRAWING_ENUMS_H

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
