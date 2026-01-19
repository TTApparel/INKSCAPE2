// SPDX-License-Identifier: GPL-2.0-or-later

#include "renderer/pixel-filters/light.h"
#include "surface-testbase.h"

using namespace Inkscape::Renderer;


template <typename Filter>
void runTest(Filter &filter, std::string cmp)
{
    auto cmyk = Colors::Manager::get().find(Colors::Space::Type::CMYK);
    auto src = TestSurface(Surface({21, 21}, 1, cmyk));
    src.rect(3, 3, 15, 15, {0.5, 0.0, 0.0, 1.0, 1.0}); 

    auto dest = TestSurface(Surface({21, 21}, 1, cmyk));

    dest.run_pixel_filter<PixelAccessEdgeMode::NO_CHECK, PixelAccessEdgeMode::ZERO>(filter, src);

    auto patch = dest.run_pixel_filter(PixelPatch(PixelPatch::Method::LIGHT));
    EXPECT_EQ(patch, PatchResult(cmp, patch._stride));
}

TEST(SurfaceLightTest, SpecularDistantLight)
{
    auto filter = PixelFilter::DistantLight(240, 20, {1.0, 1.0, 1.0, 1.0, 1.0}, 1.0, 1.0, 2.0);
    runTest(filter, "+====++"
                    "++===::"
                    "--+++::"
                    "--+++::"
                    "--+++::"
                    "-::::.:"
                    "-:::::-");
}

TEST(SurfaceLightTest, DiffusePointLight)
{
    auto filter = PixelFilter::PointLight({9, 9, 3}, 0.0, 0.0, Geom::identity(), 1, {1.0, 1.0, 1.0, 1.0, 1.0}, 1.0, 1.0);
    runTest(filter, ".      "
                    "  ..   "
                    " .:-.  "
                    " .-=.  "
                    "  ...  "
                    "       "
                    "       ");
}

TEST(SurfaceLightTest, SpecularSpotLight)
{
    auto filter = PixelFilter::SpotLight({0, 0, 9}, {15, 15, 0}, 45, 1.0, 0.0, 0.0, Geom::identity(), 1,
                                         {1.0, 1.0, 1.0, 1.0, 1.0}, 1.0, 1.0, 0.5);
    runTest(filter, " .-++++"
                    ".=oo=++"
                    "-oOOO++"
                    "+oOOO=="
                    "+=OOO=="
                    "+++==+="
                    "+++===o");
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
