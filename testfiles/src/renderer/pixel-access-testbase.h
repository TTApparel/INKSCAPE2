// SPDX-License-Identifier: GPL-2.0-or-later
/***** SHARED TEST TOOLS FOR PIXEL-ACCESS TESTS *******/

#ifndef INKSCAPE_TEST_RENDERER_PIXEL_ACCESS_TESTBASE_H
#define INKSCAPE_TEST_RENDERER_PIXEL_ACCESS_TESTBASE_H

#include <cairomm/context.h>
#include <glibmm.h>
#include <gtest/gtest.h>

#include "../test-utils.h"
#include "renderer/pixel-access.h"
#include "pixel-filter-testfilters.h"

using namespace Inkscape::Renderer;

template <int channel_count = 3, PixelAccessEdgeMode edge_mode = PixelAccessEdgeMode::NO_CHECK, cairo_format_t format = channel_count == 0 ? CAIRO_FORMAT_A8 : CAIRO_FORMAT_RGBA128F>
struct TestCairoSurface
{
    TestCairoSurface(int w, int h)
    {
        if constexpr (channel_count <= 3) {
            _cobj = {cairo_image_surface_create(format, w, h)};
            _s = {Cairo::RefPtr<Cairo::ImageSurface>(new Cairo::ImageSurface(_cobj[0], true))};
            _d = std::make_shared<PixelAccess<format, channel_count, edge_mode>>(_s[0]);
        } else if constexpr (channel_count == 4) {
            _cobj = {cairo_image_surface_create(format, w, h), cairo_image_surface_create(format, w, h)};
            _s = {Cairo::RefPtr<Cairo::ImageSurface>(new Cairo::ImageSurface(_cobj[0], true)),
                  Cairo::RefPtr<Cairo::ImageSurface>(new Cairo::ImageSurface(_cobj[1], true))};
            _d = std::make_shared<PixelAccess<format, channel_count, edge_mode>>(_s[0], _s[1]);
        }
    }

    void rect(int x, int y, int w, int h, std::array<double, channel_count + 1> const &c)
    {
        for (unsigned i = 0; i < _s.size(); i++) {
            unsigned off = i * 3;
            auto cr = Cairo::Context::create(_s[i]);
            cr->rectangle(x, y, w, h);
            cr->set_source_rgba(c[off + 0], c.size() > off + 1 && off + 1 < channel_count ? c[off + 1] : 0.0,
                                c.size() > off + 2 && off + 2 < channel_count ? c[off + 2] : 0.0, c.back());
            cr->fill();
        }
    }

    std::vector<cairo_surface_t *> _cobj; // Memory holder
    std::vector<Cairo::RefPtr<Cairo::ImageSurface>> _s;
    std::shared_ptr<PixelAccess<format, channel_count, edge_mode>> _d;
};

template <int channel_count>
struct TestCustomSurface
{
    TestCustomSurface(int w, int h)
    {
        auto custom_memory = std::vector<float>((channel_count + 1) * w * h);
        _d = std::make_shared<PixelAccess<CAIRO_FORMAT_RGBA128F, channel_count, PixelAccessEdgeMode::NO_CHECK, channel_count>>(std::move(custom_memory), w, h);
    }

    void rect(int const x, int const y, int const w, int const h, std::array<double, channel_count + 1> const &c)
    {
        for (auto x0 = x; x0 < x + w; x0++) {
            for (auto y0 = y; y0 < y + h; y0++) {
                _d->colorTo(x0, y0, c);
            }
        }
    }

    std::shared_ptr<PixelAccess<CAIRO_FORMAT_RGBA128F, channel_count, PixelAccessEdgeMode::NO_CHECK, channel_count>> _d;
};

/**
 * Test single pixel getter, double and int modes
 */
template <typename CoordType, typename Access>
::testing::AssertionResult ColorIs(Access &d, CoordType x, CoordType y, typename Access::Color const &c, bool unnmultiply = false)
{
    auto ct = d.colorAt(x, y, unnmultiply);
    return VectorIsNear(c, ct, 0.01) << "\n    X:" << x << "\n    Y:" << y << "\n\n";
}

/**
 * Test single pixel setter
 *
 * d - Surface to test
 * x,y - Coordinates to SET the color to
 * c - Color values to set
 * x2,y2 - Optional coordinates to GET where the new color will be tested (for edge testing)
 */
template <typename Access>
::testing::AssertionResult ColorWillBe(Access &d, int x, int y, typename Access::Color const &c, bool unnmultiply = false,
                                       std::optional<int> x2 = {}, std::optional<int> y2 = {})
{
    if (!x2)
        x2 = x;
    if (!y2)
        y2 = y;

    auto before = d.colorAt(*x2, *y2, unnmultiply);
    if (VectorIsNear(c, before, 0.001) == ::testing::AssertionSuccess()) {
        return ::testing::AssertionFailure() << "\n"
                                             << print_values(c) << "\n ALREADY SET at " << *x2 << "," << *y2 << "\n";
    }
    d.colorTo(x, y, c, unnmultiply);
    auto after = d.colorAt(*x2, *y2, unnmultiply);
    d.colorTo(*x2, *y2, before, unnmultiply); // Set value back
    return VectorIsNear(c, after, 0.001) << "\n    Write:" << x << "," << y << "\n    Read:" << *x2 << "," << *y2
                                         << "\n";
}

template <typename Access>
::testing::AssertionResult ImageIs(Access &d, std::string const &test, PixelPatch::Method method = PixelPatch::Method::COLORS, bool unmult = false, bool use_float = false, int patch_x = 3)
{
    auto patch = PixelPatch(method, patch_x, patch_x, unmult, use_float).filter(d);
    EXPECT_EQ(PatchResult(test, patch._stride), patch);
    if (test != patch) {
        return ::testing::AssertionFailure() << PatchResult(test, patch._stride) << "!=\n"
                                             << patch;
    }
    return ::testing::AssertionSuccess();
}

/**
 * Test the drawing surface against a compressed example
 */
::testing::AssertionResult ImageIs(Cairo::RefPtr<Cairo::ImageSurface> s, std::string const &test,
                                   PixelPatch::Method method = PixelPatch::Method::COLORS, bool unmult = false, bool use_float = false, int patch = 3)
{
    // SurfaceAccess is templated requiring compile time information about type formatting.
    switch (cairo_image_surface_get_format(s->cobj())) {
        case CAIRO_FORMAT_A8:
            {
                auto pa1 = PixelAccess<CAIRO_FORMAT_A8, 0>(s);
                return ImageIs(pa1, test, method, unmult, use_float);
            }
        case CAIRO_FORMAT_ARGB32:
            {
                auto pa2 = PixelAccess<CAIRO_FORMAT_ARGB32, 3>(s);
                return ImageIs(pa2, test, method, unmult, use_float);
            }
        case CAIRO_FORMAT_RGBA128F:
            {
                auto pa3 = PixelAccess<CAIRO_FORMAT_RGBA128F, 3>(s);
                return ImageIs(pa3, test, method, unmult, use_float);
            }
        default:
            return ::testing::AssertionFailure() << "UNHANDLED_FORMAT";
    }
}

/**
 * Get the cairo format as a printable name
 */
std::string get_format_name(cairo_format_t format)
{
    static const std::map<cairo_format_t, std::string> map = {
        {CAIRO_FORMAT_A8, "A8"},
        {CAIRO_FORMAT_ARGB32, "ARGB32"},
        {CAIRO_FORMAT_RGBA128F, "RGBA128F"},
    };
    return map.at(format);
}

template <class Filter>
::testing::AssertionResult FilterIs(Filter &&f, std::string const &test, PixelPatch::Method method = PixelPatch::Method::COLORS,
                                    bool debug = false)
{
    auto src = TestCairoSurface<4, PixelAccessEdgeMode::ZERO>(21, 21);
    src.rect(3, 3, 15, 15, {0.5, 0.0, 0.0, 1.0, 1.0});

    auto dst = TestCairoSurface<4>(21, 21);
    f.filter(*dst._d, *src._d);
    if (debug) {
        src._s[0]->write_to_png("/tmp/filter_debug_before_0.png");
        src._s[1]->write_to_png("/tmp/filter_debug_before_1.png");
        dst._s[0]->write_to_png("/tmp/filter_debug_after_0.png");
        dst._s[1]->write_to_png("/tmp/filter_debug_after_1.png");
    }
    return ImageIs(*dst._d, test, method, true);
}

template <int channel_count, class Filter>
::testing::AssertionResult FilterColors(Filter &&f, std::array<double, channel_count + 1> const &test,
                                        std::array<double, channel_count + 1> const &i1,
                                        std::optional<std::array<double, channel_count + 1>> const &i2 = {})
{
    auto src1 = TestCairoSurface<channel_count>(6, 6);
    auto src2 = TestCairoSurface<channel_count>(6, 6);
    if (i2) {
        src1.rect(0, 0, 6, 6, i1);
        src2.rect(0, 0, 6, 6, *i2);
    } else {
        src2.rect(0, 0, 6, 6, i1);
    }
    f.filter(*src1._d, *src2._d);
    auto p = src1._d->colorAt((unsigned)1, 1, true);
    return VectorIsNear(p, test, 0.001);
}

#endif // INKSCAPE_TEST_RENDERER_PIXEL_ACCESS_TESTBASE_H

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
