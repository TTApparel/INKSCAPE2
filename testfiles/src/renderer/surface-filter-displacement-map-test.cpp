// SPDX-License-Identifier: GPL-2.0-or-later

#include "colors/manager.h"
#include "renderer/pixel-filters/displacement-map.h"
#include "surface-testbase.h"

using namespace Inkscape::Renderer;

TEST(SurfaceDisplacementMapTest, OverlapEdges)
{
    // PixelAccessEdgeMode::ZERO

    auto cmyk = Colors::Manager::get().find(Colors::Space::Type::CMYK);
    auto texture = TestSurface(Surface({21, 21}, 1, cmyk));
    texture.rect(3, 3, 15, 15, {0.5, 0.0, 0.0, 1.0, 1.0});

    // This map splits off the top, bottom, left and right rows and moves them out
    auto pixels = SetPixels<4>();
    for (auto x = 0; x < 21; x++) {
        for (auto y = 0; y < 21; y++) {
            auto x1 = x / 3;
            auto y1 = y / 3;
            pixels.pixelWillBe(x, y,
                            {x1 == 0 || x1 == 5 ? 1.0 : (x1 == 1 || x1 == 6 ? 0.0 : 0.5),
                             y1 == 0 || y1 == 5 ? 1.0 : (y1 == 1 || y1 == 6 ? 0.0 : 0.5), 0.0, 1.0});
        }
    }

    auto map = TestSurface(Surface({21, 21}, 1, {}));
    map.run_pixel_filter(pixels);

    auto displace = PixelFilter::DisplacementMap(0, 1, 255 * 6, 255 * 6);
    auto dest = TestSurface(Surface({21, 21}, 1, cmyk));
    dest.run_pixel_filter<PixelAccessEdgeMode::NO_CHECK, PixelAccessEdgeMode::ZERO>(displace, texture, map);

    EXPECT_IMAGE_IS(dest,
                    "h hhh h"
                    "       "
                    "h hhh h"
                    "h hhh h"
                    "h hhh h"
                    "       "
                    "h hhh h");
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
