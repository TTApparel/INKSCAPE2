// SPDX-License-Identifier: GPL-2.0-or-later

#include "renderer/drawing-filters/tile.h"
#include "drawing-testbase.h"
#include "surface-testbase.h"

using namespace Inkscape::Renderer;

TEST(DrawingTileTest, SimpleTile)
{
    auto of = std::make_unique<DrawingFilter::Tile>();
    of->set_output(1);

    // This is much slower in linearRGB which is the default
    auto rgb = Colors::Manager::get().find(Colors::Space::Type::RGB);
    of->setInterpolationSpace(rgb);

    EXPECT_PRIMITIVE_IS(std::move(of),
                        "          "
                        "     8    "
                        "    888   "
                        "   88888  "
                        "  8888888 "
                        " 88888888 "
                        "  8888884 "
                        "   88884  "
                        "    884   "
                        "          "
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
