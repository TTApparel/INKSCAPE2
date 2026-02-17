// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Raw filter primative for convolve matrix
 *//*
 * Authors:
 *   Martin Owens
 *
 * Copyright (C) 2025 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_RENDERER_PIXEL_FILTER_CONVOLVE_MATRIX_H
#define INKSCAPE_RENDERER_PIXEL_FILTER_CONVOLVE_MATRIX_H

#include <algorithm>
#include <vector>

namespace Inkscape::Renderer::PixelFilter {

struct ConvolveMatrix
{
    ConvolveMatrix(int targetX, int targetY, int orderX, int orderY, double divisor, double bias,
                   std::vector<double> const &kernel, bool preserve_alpha)
        : _kernel(kernel.size())
        , _targetX(targetX)
        , _targetY(targetY)
        , _orderX(orderX)
        , _orderY(orderY)
        , _bias(bias)
        , _preserve_alpha(preserve_alpha)
    {
        for (unsigned i = 0; i < _kernel.size(); ++i) {
            _kernel[i] = kernel[i] / divisor;
        }
        // the matrix is given rotated 180 degrees
        // which corresponds to reverse element order
        std::reverse(_kernel.begin(), _kernel.end());
    }

    template <class AccessDst, class AccessSrc>
    void filter(AccessDst &dst, AccessSrc const &src) const
        requires (AccessSrc::checks_edge)
    {
        unsigned alpha = dst.getOutputChannels() + (_preserve_alpha ? 0 : 1);
        typename AccessDst::Color output;
        std::vector<typename AccessSrc::Color> patch(_kernel.size());

        auto width = dst.width();
        auto height = dst.height();
        auto last_line = _orderY - 1;
        auto read_pos = last_line - _targetY;

        // IF this code is too complicated, then change it to be simple get/set per pixel
        for (int x = 0; x < width; x++) {
            for (int j = 0; j < last_line; j++) {
                for (int i = 0; i < _orderX; i++) {
                    // It's ok to ask for negative coordinates, they use EdgeMode set in src
                    patch[i + j * _orderX] = src.colorAt(x + i - _targetX, j - _targetY, true);
                }
            }

            // We build the source data matrix progressively so we don't have to
            // read as many of the same pixels over and over again.
            for (int y = 0; y < height; y++) {
                // Result starts off with bias
                std::fill(output.begin(), output.end(), _bias);

                // This is the line in the patch we need to write to
                auto offset = y % _orderY;
                auto read_line = offset == 0 ? last_line : offset - 1;

                for (int i = 0; i < _orderX; i++) {
                    // May read beyond height and width, result depends on EdgeMode in src
                    patch[read_line * _orderX + i] = src.colorAt(x + i - _targetX, y + read_pos, true);
                    for (int j = 0; j < _orderY; j++) {
                        double coeff = _kernel[j * _orderX + i];
                        auto pos = ((j + offset) * _orderX + i) % patch.size();

                        // Covolve each color channel
                        for (auto k = 0; k < alpha; k++) {
                            output[k] += patch[pos][k] * coeff;
                        }
                    }
                }

                // Alpha is preserved if needed
                if (_preserve_alpha) {
                    output[alpha] = patch[((_targetY + offset) * _orderX + _targetX) % patch.size()][alpha];
                }

                // Clamp result
                for (auto k = 0; k < alpha; k++) {
                    output[k] = std::clamp(output[k], 0.0, 1.0);
                }

                // Save result to dest (hopefully not the same as src!)
                dst.colorTo(x, y, output, true);
            }
        }
    }

    std::vector<double> _kernel;
    int _targetX, _targetY, _orderX, _orderY;
    double _bias;
    bool _preserve_alpha;

    // We expect unpremultiplied Alpha
    static constexpr bool needs_unmultiplied = true;
};

} // namespace Inkscape

#endif // INKSCAPE_RENDERER_PIXEL_FILTER_CONVOLVE_MATRIX_H

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
