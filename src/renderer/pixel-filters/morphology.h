// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Raw filter functions for morpholoy filters.
 *//*
 * Authors:
 *   Felipe Corrêa da Silva Sanches
 *   Martin Owens
 *
 * Copyright (C) 2007-2025 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_RENDERER_PIXEL_FILTER_MORPHOLOGY_H
#define INKSCAPE_RENDERER_PIXEL_FILTER_MORPHOLOGY_H

#include <algorithm>
#include <cmath>
#include <deque>
#include <functional>
#include <vector>
#include <2geom/point.h>

#include "renderer/threading.h"

inline constexpr int POOL_THRESHOLD = 2048;

namespace Inkscape::Renderer::PixelFilter {

/* This performs one "half" of the morphology operation by calculating
 * the componentwise extreme in the specified axis with the given radius.
 * Extreme of row extremes is equal to the extreme of components, so this
 * doesn't change the result.
 * The algorithm is due to: Petr Dokládal, Eva Dokládalová (2011), "Computationally efficient, one-pass algorithm for
 * morphological filters"
 * TODO: Currently only the 1D algorithm is implemented, but it should not be too difficult (and at the very least more
 * memory efficient) to implement the full 2D algorithm. One problem with the 2D algorithm is that it is harder to
 * parallelize.
 */

struct Morphology
{
    bool _erode; // true: erode, false: dilate
    Geom::Point _radius;

    Morphology(bool erode, Geom::Point radius)
        : _erode(erode)
        , _radius(radius)
    {}

    // The mid aurface can be eliminnated when we have a 2d algo
    template <class AccessDst, class AccessMid, class AccessSrc>
    void filter(AccessDst &dst, AccessMid &mid, AccessSrc const &src) const
    {
        if (_erode) {
            singleAxisPass<std::less<double>, Geom::X>(mid, src);
            singleAxisPass<std::less<double>, Geom::Y>(dst, mid);
        } else {
            singleAxisPass<std::greater<double>, Geom::X>(mid, src);
            singleAxisPass<std::greater<double>, Geom::Y>(dst, mid);
        }
    }

    template <typename Comparison, Geom::Dim2 axis, class AccessDst, class AccessSrc>
    void singleAxisPass(AccessDst &dst, AccessSrc const &src) const
    {
        int channels = dst.getOutputChannels() + 1;
        Comparison comp;

        int w = dst.width();
        int h = dst.height();
        if (axis == Geom::Y)
            std::swap(w, h);

        int ri = round(_radius[axis]); // TODO: Support fractional radii?
        int wi = 2 * ri + 1;
        int const limit = w * h;

        auto const pool = get_global_dispatch_pool();
        pool->dispatch_threshold(h, limit > POOL_THRESHOLD, [&](int i, int) {
            // In tests it was actually slightly faster to allocate it here than
            // allocate it once for all threads and retrieving the correct set based
            // on the thread id.
            std::vector<std::deque<std::pair<int, double>>> vals(channels);
            typename AccessDst::Color output;

            // Initialize with transparent black
            for (int p = 0; p < AccessSrc::channel_total; ++p) {
                vals[p].emplace_back(-1, 0); // TODO: Only do this when performing an erosion?
            }
            int in_x = 0;
            int out_x = 0;

            for (int j = 0; j < w + ri; ++j) {
                for (int p = 0; p < channels; ++p) {
                    auto input = src.colorAt(axis == Geom::Y ? in_x : i, axis == Geom::Y ? i : in_x);
                    // Push new value onto FIFO, erasing any previous values that are "useless" (see paper) or
                    // out-of-range
                    if (!vals[p].empty() && vals[p].front().first + wi <= j)
                        vals[p].pop_front(); // out-of-range
                    if (j < w) {
                        while (!vals[p].empty() && !comp(vals[p].back().second, input[p]))
                            vals[p].pop_back(); // useless
                        vals[p].emplace_back(j, input[p]);
                        if (p == channels - 1)
                            in_x++;
                    } else if (j == w) { // Transparent black beyond the image. TODO: Only do this when performing an
                                         // erosion?
                        while (!vals[p].empty() && !comp(vals[p].back().second, 0))
                            vals[p].pop_back();
                        vals[p].emplace_back(j, 0);
                    }
                    // Set output
                    if (j >= ri) {
                        output[p] = vals[p].front().second;
                    }
                }
                if (j >= ri) {
                    dst.colorTo(axis == Geom::Y ? out_x : i, axis == Geom::Y ? i : out_x, output);
                    out_x++;
                }
            }
        });
    }
};

} // namespace Inkscape::Renderer::PixelFilter

#endif // INKSCAPE_RENDERER_PIXEL_FILTER_MORPHOLOGY_H

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
