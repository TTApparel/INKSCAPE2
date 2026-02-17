// SPDX-License-Identifier: GPL-2.0-or-later

#include "renderer/pixel-filters/color-matrix.h"
#include "surface-testbase.h"

using namespace Inkscape::Renderer;

TEST(SurfaceColorMatrixTest, SurfaceFilter)
{
    auto surface = TestSurface(Surface({4, 4}, 1, {}));
    surface.rect(0, 0, 4, 4, {1, 0, 1, 0.5});
    surface.run_pixel_filter(PixelFilter::ColorMatrixHueRotate(180.0), surface);
    auto color = surface.get_pixel(0, 0);
    EXPECT_TRUE(VectorIsNear(color, {0, 0.57, 0, 0.5}, 0.01));
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
