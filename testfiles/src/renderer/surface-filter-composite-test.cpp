// SPDX-License-Identifier: GPL-2.0-or-later

#include "renderer/pixel-filters/composite.h"
#include "surface-testbase.h"

using namespace Inkscape::Renderer;

TEST(SurfaceCompositeTest, Arithmetic)
{
    auto surface = TestSurface(Surface({4, 4}, 1, {}));
    surface.rect(0, 0, 4, 4, {1, 0, 0, 1});
    auto surface2 = TestSurface(Surface({4, 4}, 1, {}));
    surface2.rect(0, 0, 4, 4, {0, 1, 0, 1});
    surface.run_pixel_filter(PixelFilter::CompositeArithmetic(0.5, 0.5, 0.5, 0.5), surface2);
    auto color = surface.get_pixel(1, 1);
    EXPECT_TRUE(VectorIsNear(color, {1.0, 1.0, 0.5, 1.0}, 0.01));
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
