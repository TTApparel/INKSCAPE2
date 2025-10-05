// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Raw filter functions for color matrix transforms
 *//*
 * Authors:
 *   Felipe Corrêa da Silva Sanches
 *   Jasper van de Gronde
 *   Martin Owens
 *
 * Copyright (C) 2025 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_RENDERER_PIXEL_FILTER_MATRIX_H
#define INKSCAPE_RENDERER_PIXEL_FILTER_MATRIX_H

#include <algorithm>
#include <vector>
#include <2geom/math-utils.h>

namespace Inkscape::Renderer::PixelFilter {

struct ColorMatrixBase
{
    explicit ColorMatrixBase(double adj = 0.0)
        : _adj(adj)
    {}

    virtual std::vector<double> get_matrix(unsigned width, unsigned height) const = 0;
    static std::vector<double> pad_with_identity(std::vector<double> matrix, unsigned width, unsigned height)
    {
        // Pad the matrix with the identity.
        for (unsigned i = 0; i < height; i++) {
            // Matrix width is always number of channels plus one
            for (unsigned j = 0; j < width; j++) {
                auto k = i * width + j;
                if (k >= matrix.size()) {
                    matrix.emplace_back(i == j);
                }
            }
        }
        return matrix;
    }

    template <class AccessDst, class AccessSrc>
    void filter(AccessDst &dst, AccessSrc const &src) const
    {
        // typename AccessDst::Color out; // Compiler error
        auto out = dst.colorAt(0, 0); // Workaround

        auto matrix_width = AccessSrc::channel_total + 1;
        auto matrix_height = out.size();
        auto matrix = get_matrix(matrix_width, matrix_height);

        //dst.forEachPixel([&](int x, int y) {
        for (auto y = 0; y < dst.height(); y++) {
        for (auto x = 0; x < dst.width(); x++) {
            std::fill(out.begin(), out.end(), 0);
            auto in = src.colorAt(x, y, true);

            for (unsigned out_c = 0; out_c < matrix_height; out_c++) {
                for (unsigned in_c = 0; in_c < in.size(); in_c++) {
                    out[out_c] += in[in_c] * matrix[in_c + out_c * matrix_width];
                }
            }
            for (unsigned i = 0; i < out.size(); i++) {
                out[i] = std::clamp(out[i], 0.0, 1.0);
            }
            dst.colorTo(x, y, out, true);
        }
        }
    }

    double const _adj;
};

struct ColorMatrix : ColorMatrixBase
{
    explicit ColorMatrix(std::vector<double> matrix, double adj)
        : ColorMatrixBase(adj)
        , _matrix(std::move(matrix))
    {
    }

    std::vector<double> get_matrix(unsigned width, unsigned height) const override
    {
        return pad_with_identity(_matrix, width, height);
    }

    std::vector<double> _matrix;
};

struct ColorMatrixSaturate : ColorMatrixBase
{
    std::vector<double> get_matrix(unsigned width, unsigned height) const override
    {
        // clang-format off
        return pad_with_identity({
            // RGB Saturation matrix, doesn't work for CMYK, Gray, etc
            0.213+0.787*_v, 0.715-0.715*_v, 0.072-0.072*_v, 0.0, 0.0,
            0.213-0.213*_v, 0.715+0.285*_v, 0.072-0.072*_v, 0.0, 0.0,
            0.213-0.213*_v, 0.715-0.715*_v, 0.072+0.928*_v, 0.0, 0.0
        }, width, height);
        // clang-format on
    }

    explicit ColorMatrixSaturate(double v_in)
        : ColorMatrixBase(0.5)
        , _v(std::clamp(v_in, 0.0, 1.0))
    {}

    double _v;
};

struct ColorMatrixHueRotate : ColorMatrixBase
{
    std::vector<double> get_matrix(unsigned width, unsigned height) const override
    {
        double s, c;
        Geom::sincos(_v * M_PI / 180.0, s, c);

        // clang-format off
        return pad_with_identity({
            // RGB Hue Rotaton Matrix, won't work for other spaces like HSL
            0.213 +0.787*c -0.213*s, 0.715 -0.715*c -0.715*s, 0.072 -0.072*c +0.928*s, 0.0, 0.0,
            0.213 -0.213*c +0.143*s, 0.715 +0.285*c +0.140*s, 0.072 -0.072*c -0.283*s, 0.0, 0.0,
            0.213 -0.213*c -0.787*s, 0.715 -0.715*c +0.715*s, 0.072 +0.928*c +0.072*s, 0.0, 0.0
        }, width, height);
        // clang-format on
    }

    explicit ColorMatrixHueRotate(double v_in)
        : _v(v_in)
    {}

    double _v;
};

struct ColorMatrixLuminance : ColorMatrixBase
{
    std::vector<double> get_matrix(unsigned width, unsigned height) const override
    {
        // clang-format off
        // If we can sort out in vs. out this can be a single line matrix.
        return pad_with_identity({
            // RGB Luminance Matrix, won't work for other spaces like CMYK
            0.0,    0.0,    0.0,    0.0, 0.0,
            0.0,    0.0,    0.0,    0.0, 0.0,
            0.0,    0.0,    0.0,    0.0, 0.0,
            0.2125, 0.7154, 0.0721, 1.0, 0.0,
        }, width, height);
        // clang-format on
    }
};

} // namespace Inkscape::Renderer::PixelFilter

#endif // INKSCAPE_RENDERER_PIXEL_FILTER_MATRIX_H

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
