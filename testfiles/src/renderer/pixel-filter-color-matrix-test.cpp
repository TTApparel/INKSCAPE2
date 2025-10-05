// SPDX-License-Identifier: GPL-2.0-or-later

#include "renderer/pixel-filters/color-matrix.h"
#include "pixel-access-testbase.h"

using namespace Inkscape::Renderer::PixelFilter;

TEST(PixelColorMatrixTest, ColorMatrix)
{
    // Identity Matrix
    EXPECT_TRUE(FilterColors<3>(ColorMatrix({
                                    // clang-format off
        1, 0, 0, 0, 0,
        0, 1, 0, 0, 0,
        0, 0, 1, 0, 0,
        0, 0, 0, 1, 0
                                    // clang-format on
                                }, 0.0),
                                {1.0, 0.0, 1.0, 0.5}, {1.0, 0.0, 1.0, 0.5}));
    // Default matrix is Identity
    EXPECT_TRUE(FilterColors<3>(ColorMatrix({}, 0.0), {1.0, 0.0, 1.0, 0.5}, {1.0, 0.0, 1.0, 0.5}));
    // Morphius Matrix
    EXPECT_TRUE(FilterColors<3>(ColorMatrix({
                                    // clang-format off
        0, 0, 0, 0, 0,
        1, 1, 1, 1, 0,
        0, 0, 0, 0, 0,
        0, 0, 0, 1, 0
                                    // clang-format on
                                }, 0.0),
                                {0.0, 1.0, 0.0, 0.5}, {1.0, 0.0, 1.0, 0.5}));
}

TEST(PixelColorMatrixTest, ColorMatrixSaturate)
{
    // Testing in sRGB color space (browsers use linearRGB by default)
    EXPECT_TRUE(FilterColors<3>(ColorMatrixSaturate(0.2), {0.428, 0.228, 0.428, 0.5}, {1.0, 0.0, 1.0, 0.5}));
    EXPECT_TRUE(FilterColors<3>(ColorMatrixSaturate(0.4), {0.571, 0.171, 0.571, 0.5}, {1.0, 0.0, 1.0, 0.5}));
}

TEST(PixelColorMatrixTest, ColorMatrixHueRotate)
{
    EXPECT_TRUE(FilterColors<3>(ColorMatrixHueRotate(180.0), {0, 0.57, 0, 0.5}, {1.0, 0.0, 1.0, 0.5}));
    EXPECT_TRUE(FilterColors<3>(ColorMatrixHueRotate(90.0), {1, 0.145, 0, 0.5}, {1.0, 0.0, 1.0, 0.5}));
}

TEST(PixelColorMatrixTest, ColorMatrixLuminance)
{
    EXPECT_TRUE(FilterColors<3>(ColorMatrixLuminance(), {0, 0, 0, 0.785}, {1.0, 0.0, 1.0, 0.5}));
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
