// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Get the average color in all the pixels.
 *//*
 * Authors:
 *   Martin Owens
 *
 * Copyright (C) 2025 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_RENDERER_PIXEL_FILTER_COLOR_SPACE_H
#define INKSCAPE_RENDERER_PIXEL_FILTER_COLOR_SPACE_H

#include <iostream>

#include <optional>
#include <vector>

#include "colors/cms/transform-surface.h"
#include "colors/spaces/base.h"

namespace Inkscape::Renderer::PixelFilter {

struct AverageColor
{
    template<typename AccessSrc>
    std::vector<double> filter(AccessSrc const &src) const
    {
        double count = 0;
        std::vector<double> output(AccessSrc::channel_total, 0.0);

        for (int y = 0; y < src.height(); y++) {
            for (int x = 0; x < src.width(); x++) {
                auto px = src.colorAt(x, y, true);
                for (int c = 0; c < output.size(); c++) {
                    output[c] += px[c];
                }
                count += 1.0;
            }
        }
        for (int c = 0; c < output.size(); c++) {
            output[c] /= count;
        }
        return output;
    }

    template<typename AccessSrc, typename AccessMask>
    std::vector<double> filter(AccessSrc const &src, AccessMask const &mask) const
    {
        double count = 0;
        std::vector<double> output(AccessSrc::channel_total, 0.0);

        for (int y = 0; y < src.height(); y++) {
            for (int x = 0; x < src.width(); x++) {
                double amount = mask.alphaAt(x, y);
                // Invert the mask as requested, even if there's no mask!
                if (_invert) {
                    amount = 1.0 - amount;
                }
                auto px = src.colorAt(x, y, true);
                for (int c = 0; c < output.size(); c++) {
                    output[c] += px[c] * amount;
                }
                count += amount;
            }
        }
        for (int c = 0; c < output.size(); c++) {
            output[c] /= count;
        }
        return output;
    }

    bool _invert = false;
};

} // namespace Inkscape::Renderer::PixelFilter

#endif // INKSCAPE_RENDERER_PIXEL_FILTER_COLOR_SPACE_H

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
