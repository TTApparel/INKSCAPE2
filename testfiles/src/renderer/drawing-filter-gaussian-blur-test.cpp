// SPDX-License-Identifier: GPL-2.0-or-later

#include "renderer/drawing-filters/gaussian-blur.h"
#include "drawing-testbase.h"
#include "surface-testbase.h"

using namespace Inkscape::Renderer;

TEST(DrawingGaussianBlurTest, TransformedImage)
{
    auto blur = std::make_unique<DrawingFilter::GaussianBlur>();
    blur->set_deviation(0.02 * 1.3765, 0.001 * 1.3765);
    blur->set_output(1);
    EXPECT_PRIMITIVE_IS<PixelPatch::Method::ALPHA>(std::move(blur),
                        "          "
                        "    :-    "
                        "   :oXo   "
                        "  :oX$$o  "
                        " :oX$$$$= "
                        " -X$$$$XO."
                        "  o$$$XO: "
                        "   o$XO:  "
                        "    =O:   "
                        "     .    ");
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
