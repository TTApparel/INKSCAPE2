// SPDX-License-Identifier: GPL-2.0-or-later

#include "renderer/pixel-filters/morphology.h"
#include "pixel-access-testbase.h"

using namespace Inkscape::Renderer::PixelFilter;

TEST(PixelFilterMorphologyTest, MorphologyErode)
{
    auto src = TestCairoSurface<4, PixelAccessEdgeMode::ZERO>(21, 21);
    auto mid = TestCairoSurface<4, PixelAccessEdgeMode::ZERO>(21, 21);
    auto dst = TestCairoSurface<4, PixelAccessEdgeMode::NO_CHECK>(21, 21);

    src.rect(3, 3, 15, 15, {0.5, 0.0, 0.0, 1.0, 1.0});

    Morphology(true, {3, 3}).filter(*dst._d, *mid._d, *src._d);

    EXPECT_TRUE(ImageIs(*dst._d,
                        "       "
                        "       "
                        "  hhh  "
                        "  hhh  "
                        "  hhh  "
                        "       "
                        "       ",
                        PixelPatch::Method::COLORS, true));
}

TEST(PixelFilterMorphologyTest, MorphologyDilate)
{
    auto src = TestCairoSurface<4, PixelAccessEdgeMode::ZERO>(21, 21);
    auto mid = TestCairoSurface<4, PixelAccessEdgeMode::ZERO>(21, 21);
    auto dst = TestCairoSurface<4, PixelAccessEdgeMode::ZERO>(21, 21);

    src.rect(3, 3, 15, 15, {0.5, 0.0, 0.0, 1.0, 1.0});

    Morphology(false, {3, 3}).filter(*dst._d, *mid._d, *src._d);

    EXPECT_TRUE(ImageIs(*dst._d,
                        "hhhhhhh"
                        "hhhhhhh"
                        "hhhhhhh"
                        "hhhhhhh"
                        "hhhhhhh"
                        "hhhhhhh"
                        "hhhhhhh",
                        PixelPatch::Method::COLORS, true));

    // Erode after shouldn't leave marks
    Morphology(true, {6, 6}).filter(*src._d, *mid._d, *dst._d);

    EXPECT_TRUE(ImageIs(*src._d,
                        "       "
                        "       "
                        "  hhh  "
                        "  hhh  "
                        "  hhh  "
                        "       "
                        "       ",
                        PixelPatch::Method::COLORS, true));
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
