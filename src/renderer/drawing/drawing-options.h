// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * DrawingOptions for drawing objects
 *//*
 * Copyright (C) 2011-2026 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_INKSCAPE_RENDERER_DRAWING_OPTIONS_H
#define SEEN_INKSCAPE_RENDERER_DRAWING_OPTIONS_H

#include "colors/color.h"
#include "renderer/enums.h"

namespace Inkscape::Renderer {

/**
 * Provides a way to specify fixed options for a drawing process
 * normally from the preferences or overridden by the situation.
 */
struct DrawingOptions
{
    float device_scale = 1.0;
    std::optional<Colors::Color> outline_color = {};
    std::optional<Antialiasing> antialiasing_override = {};
    bool dithering = false;
    // Filter options
    DrawingFilter::Quality filterquality = DrawingFilter::Quality::NORMAL;
    PixelFilter::BlurQuality blurquality = PixelFilter::BlurQuality::BETTER;
};

} // end namespace Inkscape::Renderer

#endif // !SEEN_INKSCAPE_RENDERER_DRAWING_OPTIONS_H

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
