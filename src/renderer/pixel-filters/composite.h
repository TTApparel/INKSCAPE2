// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Raw filter functions for composite, most of the options are
 * handled directly by cairo, these is just the arithmetic function.
 *//*
 * Authors:
 *   Martin Owens
 *
 * Copyright (C) 2025 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_RENDERER_PIXEL_FILTER_COMPOSITE_H
#define INKSCAPE_RENDERER_PIXEL_FILTER_COMPOSITE_H

#include <algorithm>

namespace Inkscape::Renderer::PixelFilter {

struct CompositeArithmetic
{
    double _k1, _k2, _k3, _k4;

    CompositeArithmetic(double k1, double k2, double k3, double k4)
        : _k1(k1)
        , _k2(k2)
        , _k3(k3)
        , _k4(k4)
    {}

    template <class AccessDst, class AccessSrc>
    void filter(AccessDst &dst, AccessSrc const &src) const
    {
        //dst.forEachPixel([&](int x, int y) {
        for (auto y = 0; y < dst.width(); y++) {
        for (auto x = 0; x < dst.height(); x++) {
            auto c1 = dst.colorAt(x, y, true);
            auto c2 = src.colorAt(x, y, true);
            for (unsigned i = 0; i < c1.size() - 1 && i < c2.size() - 1; i++) {
                c1[i] = std::clamp(_k1 * c1[i] * c2[i] + _k2 * c1[i] + _k3 * c2[i] + _k4, 0.0, 1.0);
            }
            dst.colorTo(x, y, c1, true);
        }
        }
    }
};

} // namespace Inkscape::Renderer::PixelFilter

#endif // INKSCAPE_RENDERER_PIXEL_FILTER_COMPOSITE_H

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
