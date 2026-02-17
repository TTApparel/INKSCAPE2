// SPDX-License-Identifier: GPL-2.0-or-later
/***** TEST FILTERS *******/
  
#ifndef INKSCAPE_TEST_RENDERER_TESTFILTERS_H
#define INKSCAPE_TEST_RENDERER_TESTFILTERS_H

#include <2geom/rect.h>
#include <2geom/transforms.h>
#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

#include <iostream>

/**
 * Return a single pixel color for testing.
 */
struct SampleColor
{
    int x, y;

    template <typename AccessSrc>
    std::vector<double> filter(AccessSrc const &src) const
    {
        auto color = src.colorAt(x, y, true);
        return {color.begin(), color.end()};
    }
};

/**
 * Build a list of pixels which will be set into a surface when the
 * filter is run. Allows creating testing textures.
 *
 * All colors are NOT alpha pre-multiplied.
 */
template <int channels = 4>
struct SetPixels
{
    std::vector<std::tuple<int, int, std::array<double, channels>>> _pixels;

    void pixelWillBe(int x, int y, std::array<double, channels> color)
    {
        _pixels.emplace_back(x, y, color);
    }

    template <typename Access>
    void filter(Access &surface) const
    {
        for (auto &[x, y, color] : _pixels) {
            typename Access::Color out;
            for (auto c = 0; c < out.size() && c < color.size(); c++) {
                out[c] = color[c];
            }
            surface.colorTo(x, y, out, true);
        }
    }  
};

struct ClearPixels
{
    template <typename Access>
    void filter(Access &surface) const
    {
        typename Access::Color blank;
        for (auto y = 0; y < surface.height(); y++) {
            for (auto x = 0; x < surface.width(); x++) {
                surface.colorTo(x, y, blank);
            }
        }

    }
};

/**
 * Construct a string reprentation of the image pixels.
 */
class PatchResult : public std::string
{
public:
    unsigned _stride;

    PatchResult(std::string const &in, unsigned stride)
        : std::string(in)
        , _stride(stride)
    {}

    /**
     * Format a string into a character image for test output when failing.
     */
    friend void PrintTo(const PatchResult &obj, std::ostream *oo)
    {
        for (unsigned c = 0; c < obj.size(); c++) {
            if (obj._stride == 0 || c % obj._stride == 0) {
                if (c) 
                    *oo << "\"";
                *oo << "\n    \"";
            }
            *oo << obj[c];
        }   
        *oo << "\"\n";
    }
};

struct PixelPatch
{
    enum class Method
    {
        ALPHA,
        COLORS,
        LIGHT
    };

    Method _method;
    unsigned _patch_x = 3;
    unsigned _patch_y = 3;
    bool _alpha_unmultiplied = true;
    bool _use_float_coords = false;
    Geom::OptRect _clip;

    template<typename Access>
    PatchResult filter(Access const &src) const
    {
        static std::vector<unsigned char> const weights = {' ', ' ', ' ', '.', '.', '.', ':', ':', '-',
                                                         '+', '=', 'o', 'O', '*', 'x', 'X', '$', '&'};

        double size = _patch_x * _patch_y;
        char r0 = (_method == Method::ALPHA) ? 0x40 : 0x30;

        auto rect = Geom::Rect(0, 0, src.width(), src.height());
        if (_clip) {
            if (auto res = rect & *_clip) {
                rect = *res;
            } else {
                assert("Clipping pixel patch resulted in zero sized sample.");
            }
        }
        auto irec = *(rect * Geom::Scale(_patch_x, _patch_y).inverse()).roundInwards();

        // We collect a 3x3 grid of pixels into a patch so it can be shown as test output
        std::stringstream output;
        for (int y = irec.top(); y < irec.bottom(); y++) {
            for (int x = irec.left(); x < irec.right(); x++) {
                // inital values
                typename Access::Color colors;
                typename Access::Color lights;
                colors.fill(0.0);
                lights.fill(0.0);
  
                for (unsigned cy = 0; cy < _patch_x; cy++) {
                    for (unsigned cx = 0; cx < _patch_x; cx++) {
                        int tx = x * _patch_x + cx;
                        int ty = y * _patch_y + cy;
                        if (_method == Method::ALPHA) {
                            lights.back() += _use_float_coords ? src.alphaAt((float)tx, (float)ty) : src.alphaAt(tx, ty);
                        } else {
                            auto color = _use_float_coords ? src.colorAt((float)tx, (float)ty, _alpha_unmultiplied) : src.colorAt(tx, ty, _alpha_unmultiplied);
                            for (int c = 0; c < colors.size(); c++) {
                                colors[c] += color[c] > 0.5;
                                lights[c] += color[c];
                            }
                        }
                    }
                }
                unsigned char r = r0;
                double light = 0.0;
                for (unsigned c = 0; c < colors.size() - 1; c++) {
                    r += (unsigned char)(((colors[c] / size > 0.3) + (colors[c] / size > 0.6)) << (c * 2));
                    light += lights[c] / lights.size() / size;
                }
                switch (_method) {
                    case Method::ALPHA:
                        r = weights[(int)(lights.back() / size * (weights.size() - 1))];
                        break;
                    case Method::LIGHT:
                        r = weights[(int)(std::clamp(light, 0.0, 1.0) * (weights.size() - 1))];
                        break;
                }
                // Map zero to space for readability
                if (r == r0) {
                    r = lights.back() / size > 0.3 ? '.' : ' ';
                }
                // Cap anything higher than ascii
                while (r > 'z') {
                    r -= ('z' - r0);
                }
                output << r;
            }
        }
        return PatchResult(output.str(), irec.width());
    }
};

#endif // INKSCAPE_TEST_RENDERER_TESTFILTERS_H

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
