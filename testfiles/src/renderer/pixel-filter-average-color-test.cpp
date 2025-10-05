// SPDX-License-Identifier: GPL-2.0-or-later

#include <memory>

#include "colors/manager.h"
#include "renderer/pixel-filters/average-color.h"
#include "pixel-access-testbase.h"

using namespace Inkscape::Colors;
using namespace Inkscape::Renderer::PixelFilter;

class PixelAverageColorTest : public ::testing::Test
{
public:
    void SetUp() override
    {
        src = std::make_unique<TestCairoSurface<4>>(4, 4);
        src->rect(1, 1, 2, 2, {0.7, 0.0, 0.0, 0.0, 0.7}); // Cyan square middle
        src->rect(0, 1, 1, 2, {0.0, 0.7, 0.0, 0.0, 0.7}); // Magenta stripe left
        src->rect(3, 1, 1, 2, {0.0, 0.0, 0.7, 0.0, 0.7}); // Yellow stripe right
        src->rect(0, 0, 4, 1, {0.0, 0.0, 0.0, 1.0, 1.0}); // Black bar top
        src->rect(0, 3, 4, 1, {0.0, 0.0, 0.0, 1.0, 1.0}); // Black bar bottom
    }

    std::unique_ptr<TestCairoSurface<4>> src;
};

TEST_F(PixelAverageColorTest, AllPixelsAverageColor)
{
    // We should have twice as many Cyans and Magentas or Yellows
    // And we should have half black since that's 8 of 16 solid pixels
    ASSERT_TRUE(VectorIsNear(AverageColor().filter(*src->_d), {0.175, 0.0875, 0.0875, 0.5, 0.85}, 0.01));
}

TEST_F(PixelAverageColorTest, PixelsInsideMask)
{
    auto mask = TestCairoSurface<0, PixelAccessEdgeMode::NO_CHECK, CAIRO_FORMAT_A8>(4, 4);

    // One semi transparent square, should be no black
    mask.rect(0, 1, 4, 2, {0.5});
    EXPECT_TRUE(VectorIsNear(AverageColor().filter(*src->_d, *mask._d), {0.35,  0.175,  0.175, 0,  0.7}, 0.01));

    // Two semi transparent squares, innclude black but now a stronger cyan too
    mask.rect(1, 0, 2, 4, {0.5});
    EXPECT_TRUE(VectorIsNear(AverageColor().filter(*src->_d, *mask._d), {0.3, 0.1, 0.1, 0.29, 0.79}, 0.01));
}

TEST_F(PixelAverageColorTest, PixelsOutsideMask)
{
    auto mask = TestCairoSurface<0, PixelAccessEdgeMode::NO_CHECK, CAIRO_FORMAT_A8>(4, 4);

    mask.rect(1, 1, 2, 2, {1.0}); // Remove Cyan
    mask.rect(0, 1, 4, 2, {0.8}); // Reduce Magenta and Yellow
    EXPECT_TRUE(VectorIsNear(AverageColor(true).filter(*src->_d, *mask._d), {0, 0.03, 0.03, 0.91, 0.97}, 0.01));
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
