// SPDX-License-Identifier: GPL-2.0-or-later

#include "colors/manager.h"
#include "renderer/pixel-filters/turbulence.h"
#include "surface-testbase.h"

using namespace Inkscape::Renderer;

TEST(PixelFilterTurbulenceTest, CmykRandomness)
{
    auto cmyk = Colors::Manager::get().find(Colors::Space::Type::CMYK);
    auto src = TestSurface(Surface({21, 21}, 1, cmyk));
    auto spiky = PixelFilter::Turbulence(0,              // random generator seed
                                         {0, 0, 20, 20}, // tile size
                                         {0.6, 0.6},     // base frequency
                                         true,           // stitch
                                         false,          // fractal noise
                                         8,              // octaves
                                         5               // number of channels
    );

    spiky.setAffine(Geom::identity());
    spiky.setOrigin({0, 0});
    spiky.init();

    src.run_pixel_filter(spiky);
    EXPECT_IMAGE_IS<PixelPatch::Method::ALPHA>(src,
                    "......."
                    ".:..:.."
                    ":.-:.::"
                    ".::...."
                    ":.....:"
                    "....:.."
                    "..:.:..");
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
