// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Enumerations for various arguments in pixel filters.
 *
 *//*
 *
 * Copyright (C) 2026 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_RENDERER_PIXEL_FILTER_ENUMS_H
#define INKSCAPE_RENDERER_PIXEL_FILTER_ENUMS_H

namespace Inkscape::Renderer::PixelFilter {

enum class BlurQuality
{
    BEST = 2,
    BETTER = 1,
    NORMAL = 0,
    WORSE = -1,
    WORST = -2
};

} // namespace Inkscape::Renderer::PixelFilter

#endif // INKSCAPE_RENDERER_PIXEL_FILTER_ENUMS_H
/*
  ;Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
