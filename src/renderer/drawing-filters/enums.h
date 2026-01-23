// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors: see git history
 *
 * Copyright (C) 2014 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_INKSCAPE_RENDERER_DRAWING_FILTERS_ENUMS_H
#define SEEN_INKSCAPE_RENDERER_DRAWING_FILTERS_ENUMS_H

#include "object/sp-filter-units.h"

namespace Inkscape::Renderer::DrawingFilter {

enum class PrimitiveType
{
    BLEND,
    COLORMATRIX,
    COMPONENTTRANSFER,
    COMPOSITE,
    CONVOLVEMATRIX,
    DIFFUSELIGHTING,
    DISPLACEMENTMAP,
    DROPSHADOW,
    FLOOD,
    GAUSSIANBLUR,
    IMAGE,
    MERGE,
    MORPHOLOGY,
    OFFSET,
    SPECULARLIGHTING,
    TILE,
    TURBULENCE,
    ENDPRIMITIVETYPE // This must be last
};

enum SlotType // implicit integer, positive numbers are actual slots
{
    SLOT_NOT_SET = -1,
    SLOT_SOURCE_IMAGE = -2,
    SLOT_SOURCE_ALPHA = -3,
    SLOT_BACKGROUND_IMAGE = -4,
    SLOT_BACKGROUND_ALPHA = -5,
    SLOT_FILL_PAINT = -6,
    SLOT_STROKE_PAINT = -7,
    SLOT_RESULT = -8,
    SLOT_UNNAMED = -9
};
/* Unnamed slot is for DrawingFilter::Slot internal use. Passing it as
 * parameter to DrawingFilter::Slot accessors may have unforeseen consequences. */

enum class Quality
{
    BEST = 2,
    BETTER = 1,
    NORMAL = 0,
    WORSE = -1,
    WORST = -2
};

enum class BlurQuality
{
    BEST = 2,
    BETTER = 1,
    NORMAL = 0,
    WORSE = -1,
    WORST = -2
};


} // namespace Inkscape::Renderer::Filters

#endif // SEEN_INKSCAPE_RENDERER_DRAWING_FILTERS_ENUMS_H
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
