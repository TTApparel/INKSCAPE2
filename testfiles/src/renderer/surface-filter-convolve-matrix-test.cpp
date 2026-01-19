// SPDX-License-Identifier: GPL-2.0-or-later

#include "renderer/pixel-filters/convolve-matrix.h"
#include "surface-testbase.h"

using namespace Inkscape::Renderer;

TEST(SurfaceConvolveMatrixTest, Laplacian3x3)
{
    auto src = TestSurface(Surface({21, 21}, 1, {}));
    src.rect(3, 3, 15, 15, {0.5, 0.0, 0.0, 1.0});
    auto dest = TestSurface(Surface({21, 21}, 1, {}));

    dest.run_pixel_filter<PixelAccessEdgeMode::ZERO>(PixelFilter::ConvolveMatrix(1, 1, 3, 3, 1, 0.0, {0.0, -2.0, 0.0, -2.0, 8.0, -2.0, 0.0, -2.0, 0.0}, true), src);

    EXPECT_IMAGE_IS(dest,
                    "       "
                    " 11111 "
                    " 1...1 "
                    " 1...1 "
                    " 1...1 "
                    " 11111 "
                    "       ");

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
