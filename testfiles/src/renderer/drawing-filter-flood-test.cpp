// SPDX-License-Identifier: GPL-2.0-or-later

#include "renderer/drawing-filters/flood.h"
#include "drawing-testbase.h"
#include "surface-testbase.h"

using namespace Inkscape::Renderer;

TEST(DrawingDropShadowTest, DropShadow)
{
    auto dm = std::make_unique<DrawingFilter::Flood>();
    dm->set_output(1);
    dm->set_color(Colors::Color(0xff000088));

    // This is much slower in linearRGB which is the default
    auto rgb = Colors::Manager::get().find(Colors::Space::Type::RGB);
    dm->setInterpolationSpace(rgb);

    EXPECT_PRIMITIVE_IS(std::move(dm),
                        "2222222222"
                        "2222222222"
                        "2222222222"
                        "2222222222"
                        "2222222222"
                        "2222222222"
                        "2222222222"
                        "2222222222"
                        "2222222222"
                        "2222222222");
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
