// SPDX-License-Identifier: GPL-2.0-or-later

#include "renderer/drawing-filters/turbulence.h"
#include "drawing-testbase.h"
#include "surface-testbase.h"

using namespace Inkscape::Renderer;

TEST(DrawingTurbulenceTest, Turbulence)
{
    auto tb = std::make_unique<DrawingFilter::Turbulence>();
    tb->set_output(1);

    tb->set_seed(0);
    //tb->set_tile_size({0, 0, 20, 20});
    tb->set_baseFrequency(1.6, 1.6);
    tb->set_stitchTiles(true);
    tb->set_type(DrawingFilter::TurbulenceType::TURBULENCE);
    tb->set_numOctaves(8);

    // This is much slower in linearRGB which is the default
    auto rgb = Colors::Manager::get().find(Colors::Space::Type::RGB);
    tb->setInterpolationSpace(rgb);

    EXPECT_PRIMITIVE_IS<PixelPatch::Method::ALPHA>(std::move(tb),
                        "...   .::-"
                        "....   .::"
                        ".....   .:"
                        "......   ."
                        ".......   "
                        "........  "
                        "......... "
                        ".........."
                        ".........."
                        " ........."
    );
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
