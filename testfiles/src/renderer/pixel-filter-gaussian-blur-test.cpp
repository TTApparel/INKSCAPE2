// SPDX-License-Identifier: GPL-2.0-or-later

#include "renderer/pixel-filters/gaussian-blur.h"
#include "pixel-access-testbase.h"

using namespace Inkscape::Renderer::PixelFilter;

TEST(PixelGaussianBlurTest, GaussianBlurFIR)
{
    auto src = TestCairoSurface<3, PixelAccessEdgeMode::ZERO, CAIRO_FORMAT_ARGB32>(21, 21);
    src.rect(3, 3, 15, 15, {0.5, 0.75, 1.0, 1.0});

    GaussianBlur({2, 2}).filter(*src._d);

    EXPECT_TRUE(ImageIs(*src._d,
                        " ..... "
                        ".+OOO+."
                        ".O$$$O."
                        ".O$&$O."
                        ".O$$$O."
                        ".+OOO+."
                        " ..... ",
                        PixelPatch::Method::ALPHA, true));
}

TEST(PixelGaussianBlurTest, GaussianBlurFIRSmol)
{
    auto src = TestCairoSurface<3, PixelAccessEdgeMode::ZERO, CAIRO_FORMAT_ARGB32>(21, 21);
    src.rect(3, 3, 15, 15, {0.5, 0.75, 1.0, 1.0});

    GaussianBlur({4, 0.1}).filter(*src._d);

    EXPECT_TRUE(ImageIs(*src._d,
                        "       "
                        ".=xXx=."
                        ".=xXx=."
                        ".=xXx=."
                        ".=xXx=."
                        ".=xXx=."
                        "       ",
                        PixelPatch::Method::ALPHA, true));
}

TEST(PixelGaussianBlurTest, GaussianBlurIIR)
{
    auto src = TestCairoSurface<3, PixelAccessEdgeMode::ZERO, CAIRO_FORMAT_ARGB32>(21, 21);
    src.rect(3, 3, 15, 15, {0.5, 0.75, 1.0, 1.0});

    GaussianBlur({4, 4}).filter(*src._d);

    EXPECT_TRUE(ImageIs(*src._d,
                        " ..... "
                        ".:+=+:."
                        ".+O*O+."
                        ".=*x*=."
                        ".+O*O+."
                        ".:+=+:."
                        " ..... ",
                        PixelPatch::Method::ALPHA, true));
}


TEST(PixelGaussianBlurTest, GaussianBlurCMYK)
{
    auto src = TestCairoSurface<4, PixelAccessEdgeMode::ZERO>(21, 21);
    src.rect(3, 3, 15, 15, {0.5, 0.3, 0.0, 0.2, 1.0});

    GaussianBlur({4, 4}).filter(*src._d);

    EXPECT_TRUE(VectorIsNear(src._d->colorAt(5, 5, true), {0.5, 0.3, 0, 0.2, 0.542}, 0.01));
    EXPECT_TRUE(ImageIs(*src._d,
                        " ..... "
                        ".:+=+:."
                        ".+O*O+."
                        ".=*X*=."
                        ".+O*O+."
                        ".:+=+:."
                        " ..... ",
                        PixelPatch::Method::ALPHA, true));
}

TEST(PixelGaussianBlurTest, SpeedTest_FIR_Int)
{
    auto src = TestCairoSurface<3, PixelAccessEdgeMode::ZERO, CAIRO_FORMAT_ARGB32>(600, 600);
    src.rect(150, 100, 100, 400, {0.5, 0.75, 1.0, 1.0});
    GaussianBlur({0.0, 1}).filter(*src._d);
    EXPECT_TRUE(ImageIs(*src._d,
                        "            "
                        "            "
                        "   $$       "
                        "   &&       "
                        "   &&       "
                        "   &&       "
                        "   &&       "
                        "   &&       "
                        "   &&       "
                        "   $$       "
                        "            "
                        "            ",
                        PixelPatch::Method::ALPHA, true, false, 50));
}

TEST(PixelGaussianBlurTest, SpeedTest_FIR_Float)
{
    auto src = TestCairoSurface<3>(600, 600);
    src.rect(150, 100, 100, 400, {0.5, 0.75, 1.0, 1.0});
    GaussianBlur({0.0, 1}).filter(*src._d);
    EXPECT_TRUE(ImageIs(*src._d,
                        "            "
                        "            "
                        "   $$       "
                        "   &&       "
                        "   &&       "
                        "   &&       "
                        "   &&       "
                        "   &&       "
                        "   &&       "
                        "   $$       "
                        "            "
                        "            ",
                        PixelPatch::Method::ALPHA, true, false, 50));
}

TEST(PixelGaussianBlurTest, SpeedTest_IIR_Int)
{
    auto src = TestCairoSurface<3, PixelAccessEdgeMode::ZERO, CAIRO_FORMAT_ARGB32>(600, 600);
    src.rect(150, 100, 100, 400, {0.5, 0.75, 1.0, 1.0});
    GaussianBlur({0.0, 26.2}).filter(*src._d);
    EXPECT_TRUE(ImageIs(*src._d,
                        "            "
                        "   ..       "
                        "   **       "
                        "   $$       "
                        "   $$       "
                        "   $$       "
                        "   $$       "
                        "   $$       "
                        "   $$       "
                        "   **       "
                        "   ..       "
                        "            ",
                        PixelPatch::Method::ALPHA, true, false, 50));
}

TEST(PixelGaussianBlurTest, SpeedTest_IIR_Float)
{
    auto src = TestCairoSurface<3>(600, 600);
    src.rect(150, 100, 100, 400, {0.5, 0.75, 1.0, 1.0});
    GaussianBlur({0.0, 26.2}).filter(*src._d);
    EXPECT_TRUE(ImageIs(*src._d,
                        "            "
                        "   ..       "
                        "   **       "
                        "   $$       "
                        "   $$       "
                        "   $$       "
                        "   $$       "
                        "   $$       "
                        "   $$       "
                        "   **       "
                        "   ..       "
                        "            ",
                        PixelPatch::Method::ALPHA, true, false, 50));
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
