// SPDX-License-Identifier: GPL-2.0-or-later

#include "renderer/pixel-filters/light.h"
#include "pixel-access-testbase.h"

using namespace Inkscape::Renderer::PixelFilter;

TEST(PixelFilterLightTest, DiffuseDistantLight)
{
    EXPECT_TRUE(FilterIs(DistantLight(240, 20, {1.0, 1.0, 1.0, 1.0, 1.0}, 1.0, 1.0),
                         ".:::::."
                         ".::::.."
                         "......."
                         "......."
                         "......."
                         "..... ."
                         "......."

                         ,
                         PixelPatch::Method::LIGHT));
    // TODO: Add test for color component here
}

TEST(PixelFilterLightTest, SpecularDistantLight)
{
    EXPECT_TRUE(FilterIs(DistantLight(240, 20, {1.0, 1.0, 1.0, 1.0, 1.0}, 1.0, 1.0, 2.0),
                         "+====++"
                         "++===::"
                         "--+++::"
                         "--+++::"
                         "--+++::"
                         "-::::.:"
                         "-:::::-",
                         PixelPatch::Method::LIGHT));
}

TEST(PixelFilterLightTest, DiffusePointLight)
{
    EXPECT_TRUE(FilterIs(PointLight({9, 9, 3}, 0.0, 0.0, Geom::identity(), 1, {1.0, 1.0, 1.0, 1.0, 1.0}, 1.0, 1.0),
                         ".      "
                         "  ..   "
                         " .:-.  "
                         " .-=.  "
                         "  ...  "
                         "       "
                         "       ",
                         PixelPatch::Method::LIGHT));
}

TEST(PixelFilterLightTest, SpecularPointLight)
{
    EXPECT_TRUE(FilterIs(PointLight({9, 9, 3}, 0.0, 0.0, Geom::identity(), 1, {1.0, 1.0, 1.0, 1.0, 1.0}, 1.0, 1.0, 2.0),
                         "-::::::"
                         ":.:::.."
                         "::=o+.."
                         "::oO+.."
                         "::+++.."
                         ":......"
                         ":.....:",
                         PixelPatch::Method::LIGHT, true));
}

TEST(PixelFilterLightTest, DiffuseSpotLight)
{
    EXPECT_TRUE(FilterIs(
        SpotLight({0, 0, 9}, {15, 15, 0}, 45, 1.0, 0.0, 0.0, Geom::identity(), 1, {1.0, 1.0, 1.0, 1.0, 1.0}, 1.0, 1.0),
        "  ::..."
        " +-::.."
        ":--::.."
        "::::..."
        ".::...."
        "..... ."
        ".......",
        PixelPatch::Method::LIGHT));
}

TEST(PixelFilterLightTest, SpecularSpotLight)
{
    EXPECT_TRUE(FilterIs(SpotLight({0, 0, 9}, {15, 15, 0}, 45, 1.0, 0.0, 0.0, Geom::identity(), 1,
                                   {1.0, 1.0, 1.0, 1.0, 1.0}, 1.0, 1.0, 0.5),
                         " .-++++"
                         ".=oo=++"
                         "-oOOO++"
                         "+oOOO=="
                         "+=OOO=="
                         "+++==+="
                         "+++===o",
                         PixelPatch::Method::LIGHT));
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
