// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Raw filter functions for displacement map.
 *//*
 * Authors:
 *   Martin Owens
 *
 * Copyright (C) 2025 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_RENDERER_PIXEL_FILTER_DISPLACEMENT_MAP_H
#define INKSCAPE_RENDERER_PIXEL_FILTER_DISPLACEMENT_MAP_H

#include <algorithm>
#include <iostream>

namespace Inkscape::Renderer::PixelFilter {

struct DisplacementMap
{
    DisplacementMap(unsigned xch, unsigned ych, double scalex, double scaley)
        : _xch(xch)
        , _ych(ych)
        , _scalex(scalex / 255.0)
        , _scaley(scaley / 255.0)
    {}

    template <class AccessDst, class AccessTexture, class AccessMap>
    void filter(AccessDst &dst, AccessTexture const &texture, AccessMap const &map) const
        // Texture is allowed to request out of bounds; Set the texture's edgeMode to ZERO for SVG spec
        requires (AccessTexture::checks_edge)
    {
        // Limit problematic channel lookups in the map surface
        auto xch = std::clamp(_xch, 0, AccessMap::channel_total);
        auto ych = std::clamp(_ych, 0, AccessMap::channel_total);
        //dst.forEachPixel([&](int x, int y) {
        for (auto y = 0; y < dst.height(); y++) {
        for (auto x = 0; x < dst.width(); x++) {
            auto mappx = map.colorAt(x, y, true);
            auto tx = texture.colorAt(x + _scalex * (mappx[xch] - 0.5), y + _scaley * (mappx[ych] - 0.5), true);
            // TODO: Replace this when we can compile time check the size of Access
            typename AccessDst::Color out;
            for (auto c = 0; c < out.size() && c < tx.size(); c++) {
                out[c] = tx[c];
            }
            dst.colorTo(x, y, out, true);
        }
        }
    }

    int _xch, _ych;
    double _scalex, _scaley;
};

} // namespace Inkscape::Renderer::PixelFilter

#endif // INKSCAPE_RENDERER_PIXEL_FILTER_DISPLACEMENT_MAP_H

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
