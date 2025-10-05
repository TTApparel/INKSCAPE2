// SPDX-License-Identifier: GPL-2.0-or-later

#include "renderer/pixel-filters/convolve-matrix.h"
#include "pixel-access-testbase.h"

using namespace Inkscape::Renderer::PixelFilter;

/*
 * Filer is done on the same image of a square defined in pixel-access-testbase.h
 */

TEST(PixelFilterConvolveMatrixTest, Laplacian3x3)
{
    EXPECT_TRUE(FilterIs(ConvolveMatrix(1, 1, 3, 3, 1, 0.0, {
         0.0, -2.0,  0.0,
        -2.0,  8.0, -2.0,
         0.0, -2.0,  0.0}, true),
                         "       "
                         " qqqqq "
                         " q...q "
                         " q...q "
                         " q...q "
                         " qqqqq "
                         "       "));
}

TEST(PixelFilterConvolveMatrixTest, Laplacian5x5)
{
    EXPECT_TRUE(FilterIs(
        ConvolveMatrix(3, 3, 5, 5, 1, 0.0,
                       {0, 0, -1, 0, 0, 0, -1, -2, -1, 0, -1, -2, 16, -2, -1, 0, -1, -2, -1, 0, 0, 0, -1, 0, 0}, true),
        "       "
        " qhhhh "
        " h...q "
        " h...q "
        " h...q "
        " hqqqq "
        "       "));
}

TEST(PixelFilterConvolveMatrixTest, Prewitt)
{
    // Tests direction bias in matrix
    EXPECT_TRUE(FilterIs(ConvolveMatrix(1, 1, 3, 3, 1, 0.0, {-1.0, 0.0, 1.0, -1.0, 0.0, 1.0, -1.0, 0.0, 1.0}, true),
                         "       "
                         " ....q "
                         " ....q "
                         " ....q "
                         " ....q "
                         " ....q "
                         "       "));
    EXPECT_TRUE(FilterIs(ConvolveMatrix(1, 1, 3, 3, 1, 0.0, {1.0, 0.0, -1.0, 1.0, 0.0, -1.0, 1.0, 0.0, -1.0}, true),
                         "       "
                         " q.... "
                         " q.... "
                         " q.... "
                         " q.... "
                         " q.... "
                         "       "));
    EXPECT_TRUE(FilterIs(ConvolveMatrix(1, 1, 3, 3, 1, 0.0, {-1.0, -1.0, -1.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0}, true),
                         "       "
                         " ..... "
                         " ..... "
                         " ..... "
                         " ..... "
                         " qqqqq "
                         "       "));
    EXPECT_TRUE(FilterIs(ConvolveMatrix(1, 1, 3, 3, 1, 0.0, {1.0, 1.0, 1.0, 0.0, 0.0, 0.0, -1.0, -1.0, -1.0}, true),
                         "       "
                         " qqqqq "
                         " ..... "
                         " ..... "
                         " ..... "
                         " ..... "
                         "       "));
}

TEST(PixelFilterConvolveMatrixTest, ElongatedKernel)
{
    EXPECT_TRUE(FilterIs(ConvolveMatrix(4, 1, 9, 3, 1, 0.0, {1.0, 1.0, 1.0, 1.0, 0.0, -1.0, -1.0, -1.0, -1.0,
                                                             1.0, 1.0, 1.0, 1.0, 0.0, -1.0, -1.0, -1.0, -1.0,
                                                             1.0, 1.0, 1.0, 1.0, 0.0, -1.0, -1.0, -1.0, -1.0},
                                        true),
                         "       "
                         " hq... "
                         " hq... "
                         " hq... "
                         " hq... "
                         " hq... "
                         "       "));
}
TEST(PixelFilterConvolveMatrixTest, PreserveAlpha)
{
    EXPECT_TRUE(FilterIs(ConvolveMatrix(1, 1, 3, 3, 1, 0.0, {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}, false),
                         "       "
                         "       "
                         "       "
                         "       "
                         "       "
                         "       "
                         "       "));
    EXPECT_TRUE(FilterIs(ConvolveMatrix(1, 1, 3, 3, 1, 0.0, {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}, true),
                         "       "
                         " ..... "
                         " ..... "
                         " ..... "
                         " ..... "
                         " ..... "
                         "       "));
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
