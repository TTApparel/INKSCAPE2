// SPDX-License-Identifier: GPL-2.0-or-later

#include <gtest/gtest.h>

#include "colors/manager.h"

#include "surface-testbase.h"

// This is a UNIT TEST internal function
TEST(DrawingSurfaceTest, ReadFromPNG)
{
    auto surface = TestSurface(Renderer::Surface(INKSCAPE_TESTS_DIR "/data/renderer/transform-source-16.png"));

    EXPECT_IMAGE_IS<PixelPatch::Method::ALPHA>(surface,
                    "        "
                    "   :*   "
                    "  :$&*  "
                    " :$&&&* "
                    " *&&&&$."
                    "  *&&$. "
                    "   *$.  "
                    "    .   "
                  , 50);

    auto surface2 = TestSurface(Renderer::Surface(INKSCAPE_TESTS_DIR "/data/renderer/transform-slot-tr.png"));

    EXPECT_IMAGE_IS<PixelPatch::Method::ALPHA>(surface2,
                    "        "
                    "  ::::  "
                    " :&&&&: "
                    " :&&&&: "
                    " :&&&&: "
                    " :&&&&: "
                    "  ::::  "
                    "        "
                  , 50, {100, 100, 540, 540});
}

TEST(DrawingSurfaceTest, AlphaSurface)
{
    auto alpha = Colors::Manager::get().find(Colors::Space::Type::Alpha);
    auto surface = TestSurface(Renderer::Surface({10, 10}, 1, alpha));

    auto surfaces = surface.testCairoSurfaces();
    ASSERT_TRUE(surface.ready());
    ASSERT_EQ(surfaces.size(), 1);
    ASSERT_EQ(cairo_image_surface_get_format(surfaces[0]->cobj()), CAIRO_FORMAT_A8);

    auto similar = surface.similar(Geom::IntPoint(20, 20));
    ASSERT_EQ(similar->getCairoSurfaces().size(), surface.getCairoSurfaces().size());
    ASSERT_EQ(similar->format(), surface.format());
    ASSERT_EQ(similar->getColorSpace(), surface.getColorSpace());
}

TEST(DrawingSurfaceTest, sRGBIntegerSurface)
{
    // sRGB Integer Format
    auto surface = TestSurface(Renderer::Surface({10, 10}, 1, {}));

    ASSERT_FALSE(surface.ready());
    ASSERT_EQ(surface.dimensions(), Geom::IntPoint(10, 10));

    auto surfaces = surface.testCairoSurfaces();
    ASSERT_TRUE(surface.ready());
    ASSERT_EQ(surfaces.size(), 1);
    ASSERT_EQ(cairo_image_surface_get_format(surfaces[0]->cobj()), CAIRO_FORMAT_ARGB32);

    auto similar = surface.similar(Geom::IntPoint(20, 20));
    ASSERT_EQ(similar->getCairoSurfaces().size(), surface.getCairoSurfaces().size());
    ASSERT_EQ(similar->format(), surface.format());
    ASSERT_EQ(similar->getColorSpace(), surface.getColorSpace());
}

TEST(DrawingSurfaceTest, RGBFloatSurface)
{
    auto rgb = Colors::Manager::get().find(Colors::Space::Type::RGB);
    auto surface = TestSurface(Renderer::Surface({10, 10}, 1, rgb));

    auto surfaces = surface.testCairoSurfaces();
    ASSERT_TRUE(surface.ready());
    ASSERT_EQ(surfaces.size(), 1);
    ASSERT_EQ(cairo_image_surface_get_format(surfaces[0]->cobj()), CAIRO_FORMAT_RGBA128F);

    auto similar = surface.similar(Geom::IntPoint(20, 20));
    ASSERT_EQ(similar->getCairoSurfaces().size(), surfaces.size());
    ASSERT_EQ(similar->format(), surface.format());
    ASSERT_EQ(similar->getColorSpace(), surface.getColorSpace());
}

TEST(DrawingSurfaceTest, CMYKFloatSurface)
{
    auto cmyk = Colors::Manager::get().find(Colors::Space::Type::CMYK);
    auto surface = TestSurface(Renderer::Surface({10, 10}, 1, cmyk));

    auto surfaces = surface.testCairoSurfaces();
    ASSERT_TRUE(surface.ready());
    ASSERT_EQ(surfaces.size(), 2);
    // This says RGBA, but it's actually CMYA
    ASSERT_EQ(cairo_image_surface_get_format(surfaces[0]->cobj()), CAIRO_FORMAT_RGBA128F);
    // This says RGBA, but it's actually K--A
    ASSERT_EQ(cairo_image_surface_get_format(surfaces[1]->cobj()), CAIRO_FORMAT_RGBA128F);

    auto similar = surface.similar(Geom::IntPoint(20, 20));
    ASSERT_EQ(similar->format(), surface.format());
    ASSERT_EQ(similar->getCairoSurfaces().size(), surface.getCairoSurfaces().size());
    ASSERT_EQ(similar->getColorSpace(), surface.getColorSpace());
}

struct TestPixelFilter
{
    constexpr static bool edge_check = false;

    std::vector<double> failure() const
    {
        return {};
    }

    template <typename AccessDst>
    std::vector<double> filter(AccessDst &dst) const
    {
        typename AccessDst::Color color;
        for (unsigned c = 0; c < color.size(); c++) {
            color[c] = 1.0;
        }
        dst.colorTo(0, 0, color);
        return {color.begin(), color.end()};
    }
    template <typename AccessDst, typename AccessSrc>
    std::vector<double> filter(AccessDst &dst, AccessSrc const &src) const
    {
        auto color1 = src.colorAt(0, 0);
        typename AccessDst::Color color;
        for (unsigned c = 0; c < color.size() && c < color1.size(); c++) {
            color[c] = color1[c] + 1.5;
        }
        dst.colorTo(0, 0, color);
        return {color.begin(), color.end()};
    }
    template <typename AccessDst, typename AccessSrc, typename AccessMask>
    std::vector<double> filter(AccessDst &dst, AccessSrc const &src, AccessMask const &mask) const
    {
        auto color1 = src.colorAt(0, 0);
        auto color2 = mask.colorAt(0, 0);
        typename AccessDst::Color color;
        for (unsigned c = 0; c < color.size() && c < color1.size() && c < color2.size(); c++) {
            color[c] = color1[c] + color2[c];
        }
        dst.colorTo(0, 0, color);
        return {color.begin(), color.end()};
    }
};

TEST(DrawingSurfaceTest, RunPixelFilter)
{
    auto filter1 = TestPixelFilter();

    auto rgb32_surface = TestSurface(Renderer::Surface({10, 10}, 1, {}));
    { // Int RGB
        auto color = rgb32_surface.run_pixel_filter(filter1);
        ASSERT_TRUE(VectorIsNear(color, {1.0, 1.0, 1.0, 1.0}, 0.01));
    }

    auto rgb = Colors::Manager::get().find(Colors::Space::Type::RGB);
    auto c3f_surface = TestSurface(Renderer::Surface({10, 10}, 1, rgb));
    { // Float 3 channel
        auto color = c3f_surface.run_pixel_filter(filter1);
        ASSERT_TRUE(VectorIsNear(color, {1.0, 1.0, 1.0, 1.0}, 0.01));
    }

    auto cmyk = Colors::Manager::get().find(Colors::Space::Type::CMYK);
    auto c4f_surface = TestSurface(Renderer::Surface({10, 10}, 1, cmyk));
    { // Float 4 channel
        auto color = c4f_surface.run_pixel_filter(filter1);
        ASSERT_TRUE(VectorIsNear(color, {1.0, 1.0, 1.0, 1.0, 1.0}, 0.01));
    }

    { // Src and Dst
        auto color = c4f_surface.run_pixel_filter(filter1, c3f_surface);
        ASSERT_TRUE(VectorIsNear(color, {2.5, 2.5, 2.5, 2.5, 0}, 0.01));
    }

    { // Src, Dst and Mask
        auto color = c4f_surface.run_pixel_filter(filter1, c3f_surface, rgb32_surface);
        ASSERT_TRUE(VectorIsNear(color, {2.0, 2.0, 2.0, 2.0, 0}, 0.01));
    }
}

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
