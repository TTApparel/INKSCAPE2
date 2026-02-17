// SPDX-License-Identifier: GPL-2.0-or-later

#include "renderer/pixel-filters/morphology.h"
#include "surface-testbase.h"

using namespace Inkscape::Renderer;

TEST(PixelFilterMorphology, MorphologyErode)
{
    auto cmyk = Colors::Manager::get().find(Colors::Space::Type::CMYK);
    auto src = TestSurface(Surface({21, 21}, 1, cmyk));
    auto mid = TestSurface(Surface({21, 21}, 1, cmyk));
    auto dst = TestSurface(Surface({21, 21}, 1, cmyk));

    src.rect(3, 3, 15, 15, {0.5, 0.0, 0.0, 1.0, 1.0});

    dst.run_pixel_filter<PixelAccessEdgeMode::NO_CHECK, PixelAccessEdgeMode::ZERO>(PixelFilter::Morphology(true, {3, 3}), mid, src);
    auto result = dst.run_pixel_filter(PixelPatch(PixelPatch::Method::COLORS));
    EXPECT_EQ(result,
              "       "
              "       "
              "  hhh  "
              "  hhh  "
              "  hhh  "
              "       "
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
