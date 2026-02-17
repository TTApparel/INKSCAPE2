// SPDX-License-Identifier: GPL-2.0-or-later

#include "renderer/drawing-filters/blend.h"
#include "drawing-testbase.h"
#include "surface-testbase.h"

using namespace Inkscape::Renderer;

TEST(DrawingBlendTest, TransformedImage)
{
    auto blend = std::make_unique<DrawingFilter::Blend>();
    blend->set_input(1, DrawingFilter::SLOT_BACKGROUND_IMAGE);
    blend->set_output(1);
    blend->set_mode(SP_CSS_BLEND_MULTIPLY);

    // This is much slower in linearRGB which is the default
    auto rgb = Colors::Manager::get().find(Colors::Space::Type::RGB);
    blend->setInterpolationSpace(rgb);

    EXPECT_PRIMITIVE_IS(std::move(blend),
                        "RRRRRRRRRR"
                        "QQQQQ@QQQQ"
                        "PPPP..@PPP"
                        "XXX....DXX"
                        "HH8888888H"
                        "8888888888"
                        "9988888899"
                        ":::9888:::"
                        "::::1.::::"
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
