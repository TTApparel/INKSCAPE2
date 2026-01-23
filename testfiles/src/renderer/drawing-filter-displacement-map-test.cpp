// SPDX-License-Identifier: GPL-2.0-or-later

#include "renderer/drawing-filters/displacement-map.h"
#include "drawing-testbase.h"
#include "surface-testbase.h"

using namespace Inkscape::Renderer;

TEST(DrawingDisplacementMapTest, BananaMap)
{
    auto dm = std::make_unique<DrawingFilter::DisplacementMap>();
    dm->set_output(1);

    dm->set_channels(1, 2);
    dm->set_scale(40);
    dm->set_input(0, DrawingFilter::SLOT_SOURCE_IMAGE);
    dm->set_input(1, DrawingFilter::SLOT_BACKGROUND_IMAGE);

    // This is much slower in linearRGB which is the default
    auto rgb = Colors::Manager::get().find(Colors::Space::Type::RGB);
    dm->setInterpolationSpace(rgb);

    EXPECT_PRIMITIVE_IS(std::move(dm),
                          "          "
                          "       88 "
                          "    488888"
                          "  88888884"
                          "88888884  "
                          "888888    "
                          "88888     "
                          "88884     "
                          " 88884    "
                          "  88888   ");
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
