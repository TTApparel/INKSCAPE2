// SPDX-License-Identifier: GPL-2.0-or-later

#include "renderer/pixel-filters/turbulence.h"
#include "pixel-access-testbase.h"

using namespace Inkscape::Renderer::PixelFilter;

TEST(PixelFilterTurbulenceTest, CmykTurbulence)
{
    auto surface = TestCairoSurface<4>(21, 21);
    auto spiky = Turbulence(0,              // random generator seed
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

    spiky.filter(*surface._d);
    EXPECT_TRUE(ImageIs(*surface._d,
                        "......."
                        ".:..:.."
                        ":.-:.::"
                        ".::...."
                        ":.....:"
                        "....:.."
                        "..:.:..",
                        PixelPatch::Method::ALPHA));
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
