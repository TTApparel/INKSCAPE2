// SPDX-License-Identifier: GPL-2.0-or-later

#include "renderer/pixel-filters/component-transfer.h"
#include "surface-testbase.h"

using namespace Inkscape::Renderer;

TEST(SurfaceComponentTransferTest, ComponentTransferGamma)
{
    std::vector<PixelFilter::TransferFunction> tfs = {
        PixelFilter::TransferFunction(4.0, 7.0, 0.0),
        PixelFilter::TransferFunction(4.0, 4.0, 0.0),
        PixelFilter::TransferFunction(4.0, 1.0, 0.0),
    };

    auto surface = TestSurface(Surface({4, 4}, 1, {}));
    surface.rect(0, 0, 4, 4, {1.0, 0.5, 0.0, 1.0});
    surface.run_pixel_filter(PixelFilter::ComponentTransfer(tfs), surface);
    auto color = surface.get_pixel(1, 1);
    EXPECT_TRUE(VectorIsNear(color, {1.0, 0.25, 0.0, 1.0}, 0.01));
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
