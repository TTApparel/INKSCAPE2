// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Raw filter functions for turbulence and fractal noise
 *//*
 * Authors:
 *   Felipe Corrêa da Silva Sanches
 *   Martin Owens
 *
 * This file has a considerable amount of code adapted from
 *  the W3C SVG filter specs, available at:
 *  http://www.w3.org/TR/SVG11/filters.html#feTurbulence
 *
 * W3C original code is licensed under the terms of
 *  the (GPL compatible) W3C® SOFTWARE NOTICE AND LICENSE:
 *  http://www.w3.org/Consortium/Legal/2002/copyright-software-20021231
 *
 * Copyright (C) 2007-2025 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef INKSCAPE_RENDERER_PIXEL_FILTER_TURBULENCE_H
#define INKSCAPE_RENDERER_PIXEL_FILTER_TURBULENCE_H

#include <vector>
#include <2geom/point.h>
#include <2geom/rect.h>

namespace Inkscape::Renderer::PixelFilter {

class Turbulence
{
public:
    Turbulence(long seed, Geom::Rect const &tile, Geom::Point const &freq, bool stitch, bool fractalnoise, int octaves,
               int channels = 4)
        : _seed(seed)
        , _tile(tile)
        , _baseFreq(freq)
        , _stitchTiles(stitch)
        , _fractalnoise(fractalnoise)
        , _octaves(octaves)
        , _channels(channels)
        , _latticeSelector()
        , _wrapx(0)
        , _wrapy(0)
        , _wrapw(0)
        , _wraph(0)
    {}

    void setSeed(long seed)
    {
        _seed = seed;
        _ready = false;
    }
    // Why no setTile() ?
    void setbaseFrequency(Geom::Dim2 axis, double freq)
    {
        _baseFreq[axis] = freq;
        _ready = false;
    }
    void setOctaves(int octaves)
    {
        _octaves = octaves;
        _ready = false;
    }
    void setStitchTiles(bool stitch)
    {
        _stitchTiles = stitch;
        _ready = false;
    }
    void setFractalnoise(bool fractalnoise)
    {
        _fractalnoise = fractalnoise;
        _ready = false;
    }
    void setChannels(int channels)
    {
        _channels = channels;
        _ready = false;
    }

    void setAffine(Geom::Affine tr) { _affine = tr; }
    void setOrigin(Geom::IntPoint p) { _origin = p; }

    template <class AccessDst>
    void filter(AccessDst &dst) const
    {
        assert(_ready);

        typename AccessDst::Color output;

        for (int y = 0; y < dst.height(); y++) {
            for (int x = 0; x < dst.width(); x++) {
                // transform is added now to keep randomness the same regardless
                // of how the surface may have been transformed.
                turbulencePixel<AccessDst::channel_total>(Geom::Point(x + _origin[Geom::X], y + _origin[Geom::Y]) * _affine, output);
                dst.colorTo(x, y, output, true);
            }
        }
    }

    void init()
    {
        if (_ready)
            return;

        // setup random number generator
        _setupSeed(_seed);

        // Prep gradient memory
        for (auto i = 0; i < 2 * BSize + 2; ++i) {
            _gradient[i][0] = std::vector<double>(_channels, 0.0);
            _gradient[i][1] = std::vector<double>(_channels, 0.0);
        }

        int i;
        for (int k = 0; k < _channels; ++k) {
            for (i = 0; i < BSize; ++i) {
                _latticeSelector[i] = i;

                do {
                    _gradient[i][0][k] = static_cast<double>(_random() % (BSize * 2) - BSize) / BSize;
                    _gradient[i][1][k] = static_cast<double>(_random() % (BSize * 2) - BSize) / BSize;
                } while (_gradient[i][0][k] == 0 && _gradient[i][1][k] == 0);

                // normalize gradient
                double s = hypot(_gradient[i][0][k], _gradient[i][1][k]);
                _gradient[i][0][k] /= s;
                _gradient[i][1][k] /= s;
            }
        }
        while (--i) {
            // shuffle lattice selectors
            int j = _random() % BSize;
            std::swap(_latticeSelector[i], _latticeSelector[j]);
        }

        // fill out the remaining part of the gradient
        for (i = 0; i < BSize + 2; ++i) {
            _latticeSelector[BSize + i] = _latticeSelector[i];

            for (int k = 0; k < _channels; ++k) {
                _gradient[BSize + i][0][k] = _gradient[i][0][k];
                _gradient[BSize + i][1][k] = _gradient[i][1][k];
            }
        }

        // When stitching tiled turbulence, the frequencies must be adjusted
        // so that the tile borders will be continuous.
        if (_stitchTiles) {
            if (_baseFreq[Geom::X] != 0.0) {
                double freq = _baseFreq[Geom::X];
                double lo = std::floor(_tile.width() * freq) / _tile.width();
                double hi = std::ceil(_tile.width() * freq) / _tile.width();
                _baseFreq[Geom::X] = freq / lo < hi / freq ? lo : hi;
            }
            if (_baseFreq[Geom::Y] != 0.0) {
                double freq = _baseFreq[Geom::Y];
                double lo = std::floor(_tile.height() * freq) / _tile.height();
                double hi = std::ceil(_tile.height() * freq) / _tile.height();
                _baseFreq[Geom::Y] = freq / lo < hi / freq ? lo : hi;
            }

            _wrapw = _tile.width() * _baseFreq[Geom::X] + 0.5;
            _wraph = _tile.height() * _baseFreq[Geom::Y] + 0.5;
            _wrapx = _tile.left() * _baseFreq[Geom::X] + PerlinOffset + _wrapw;
            _wrapy = _tile.top() * _baseFreq[Geom::Y] + PerlinOffset + _wraph;
        }
        _ready = true;
    }

    template <int channel_total>
    inline void turbulencePixel(Geom::Point const &point, std::array<double, channel_total> &output) const
    {
        std::fill(output.begin(), output.end(), 0.0);
        int wrapx = _wrapx, wrapy = _wrapy, wrapw = _wrapw, wraph = _wraph;

        double x = point[Geom::X] * _baseFreq[Geom::X];
        double y = point[Geom::Y] * _baseFreq[Geom::Y];
        double ratio = 1.0;

        for (int octave = 0; octave < _octaves; ++octave) {
            double tx = x + PerlinOffset;
            double bx = floor(tx);
            double rx0 = tx - bx, rx1 = rx0 - 1.0;
            int bx0 = bx, bx1 = bx0 + 1;

            double ty = y + PerlinOffset;
            double by = floor(ty);
            double ry0 = ty - by, ry1 = ry0 - 1.0;
            int by0 = by, by1 = by0 + 1;

            if (_stitchTiles) {
                if (bx0 >= wrapx)
                    bx0 -= wrapw;
                if (bx1 >= wrapx)
                    bx1 -= wrapw;
                if (by0 >= wrapy)
                    by0 -= wraph;
                if (by1 >= wrapy)
                    by1 -= wraph;
            }
            bx0 &= BMask;
            bx1 &= BMask;
            by0 &= BMask;
            by1 &= BMask;

            int i = _latticeSelector[bx0];
            int j = _latticeSelector[bx1];
            int b00 = _latticeSelector[i + by0];
            int b01 = _latticeSelector[i + by1];
            int b10 = _latticeSelector[j + by0];
            int b11 = _latticeSelector[j + by1];

            double sx = _scurve(rx0);
            double sy = _scurve(ry0);

            auto const *qxa = _gradient[b00];
            auto const *qxb = _gradient[b10];
            auto const *qya = _gradient[b01];
            auto const *qyb = _gradient[b11];
            for (int k = 0; k < channel_total; ++k) {
                double a = _lerp(sx, rx0 * qxa[0][k] + ry0 * qxa[1][k], rx1 * qxb[0][k] + ry0 * qxb[1][k]);
                double b = _lerp(sx, rx0 * qya[0][k] + ry1 * qya[1][k], rx1 * qyb[0][k] + ry1 * qyb[1][k]);
                double r = _lerp(sy, a, b);
                output[k] += _fractalnoise ? r / ratio : fabs(r) / ratio;
            }

            x *= 2;
            y *= 2;
            ratio *= 2;

            if (_stitchTiles) {
                // Update stitch values. Subtracting PerlinOffset before the multiplication and
                // adding it afterward simplifies to subtracting it once.
                wrapw *= 2;
                wraph *= 2;
                wrapx = wrapx * 2 - PerlinOffset;
                wrapy = wrapy * 2 - PerlinOffset;
            }
        }

        for (auto i = 0; i < channel_total; i++) {
            if (_fractalnoise) {
                output[i] += 1;
                output[i] /= 2;
            }
            output[i] = std::clamp(output[i], 0.0, 1.0);
        }
    }

private:
    void _setupSeed(long seed)
    {
        _seed = seed;
        if (_seed <= 0)
            _seed = -(_seed % (RAND_m - 1)) + 1;
        if (_seed > RAND_m - 1)
            _seed = RAND_m - 1;
    }

    long _random()
    {
        /* Produces results in the range [1, 2**31 - 2].
         * Algorithm is: r = (a * r) mod m
         * where a = 16807 and m = 2**31 - 1 = 2147483647
         * See [Park & Miller], CACM vol. 31 no. 10 p. 1195, Oct. 1988
         * To test: the algorithm should produce the result 1043618065
         * as the 10,000th generated number if the original seed is 1. */
        _seed = RAND_a * (_seed % RAND_q) - RAND_r * (_seed / RAND_q);
        if (_seed <= 0)
            _seed += RAND_m;
        return _seed;
    }

    static inline double _scurve(double t) { return t * t * (3.0 - 2.0 * t); }

    static inline double _lerp(double t, double a, double b) { return a + t * (b - a); }

    // random number generator constants
    static long constexpr RAND_m = 2147483647, // 2**31 - 1
        RAND_a = 16807,                        // 7**5; primitive root of m
        RAND_q = 127773,                       // m / a
        RAND_r = 2836;                         // m % a

    // other constants
    static int constexpr BSize = 0x100;
    static int constexpr BMask = 0xff;

    static double constexpr PerlinOffset = 4096.0;

    // Input arguments
    long _seed;
    Geom::Rect _tile;
    Geom::Point _baseFreq;
    bool _stitchTiles;
    bool _fractalnoise;
    int _octaves;
    int _channels;

    // Generated in init
    int _latticeSelector[2 * BSize + 2];
    std::vector<double> _gradient[2 * BSize + 2][2];
    int _wrapx;
    int _wrapy;
    int _wrapw;
    int _wraph;
    bool _ready = false;

    Geom::Affine _affine;
    Geom::IntPoint _origin;
};

} // namespace Inkscape::Renderer::PixelFilter

#endif // INKSCAPE_RENDERER_PIXEL_FILTER_TURBULENCE_H

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
