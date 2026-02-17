// SPDX-License-Identifier: GPL-2.0-or-later

#include <memory>

#include "colors/manager.h"
#include "renderer/pixel-filters/average-color.h"
#include "surface-testbase.h"

using namespace Inkscape::Renderer;

class SurfaceAverageColorTest : public ::testing::Test
{
public:
    void SetUp() override
    {
        auto cmyk = Colors::Manager::get().find(Colors::Space::Type::CMYK);
        src = std::make_unique<TestSurface>(Renderer::Surface({4, 4}, 1, cmyk));
        src->rect(1, 1, 2, 2, {0.7, 0.0, 0.0, 0.0, 0.7}); // Cyan square middle
        src->rect(0, 1, 1, 2, {0.0, 0.7, 0.0, 0.0, 0.7}); // Magenta stripe left
        src->rect(3, 1, 1, 2, {0.0, 0.0, 0.7, 0.0, 0.7}); // Yellow stripe right
        src->rect(0, 0, 4, 1, {0.0, 0.0, 0.0, 1.0, 1.0}); // Black bar top
        src->rect(0, 3, 4, 1, {0.0, 0.0, 0.0, 1.0, 1.0}); // Black bar bottom
    }

    std::unique_ptr<TestSurface> src;
};

TEST_F(SurfaceAverageColorTest, SurfaceFilterNoMask)
{
    auto color = src->run_pixel_filter(PixelFilter::AverageColor());
    EXPECT_TRUE(VectorIsNear(color, {0.175, 0.0875, 0.0875, 0.5, 0.85}, 0.01));
}

TEST_F(SurfaceAverageColorTest, SurfaceFilterMask)
{
    auto alpha = Colors::Manager::get().find(Colors::Space::Type::Alpha);
    auto mask = TestSurface(Renderer::Surface({4, 4}, 1, alpha));
    mask.rect(0, 1, 4, 2, {0.5});

    auto color = src->run_pixel_filter(PixelFilter::AverageColor(), mask);
    EXPECT_TRUE(VectorIsNear(color, {0.35, 0.175, 0.175, 0, 0.7}, 0.01));
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
