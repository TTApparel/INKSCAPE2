// SPDX-License-Identifier: GPL-2.0-or-later

#include "renderer/drawing-filters/convolve-matrix.h"
#include "drawing-testbase.h"
#include "surface-testbase.h"

using namespace Inkscape::Renderer;

TEST(DrawingConvolveMatrixTest, MatrixTable)
{
    auto cvm = std::make_unique<DrawingFilter::ConvolveMatrix>();
    cvm->set_output(1);

    cvm->set_targetX(1);
    cvm->set_targetY(1);
    cvm->set_orderX(3);
    cvm->set_orderY(3);
    cvm->set_divisor(1);
    cvm->set_bias(0.0);
    cvm->set_kernelMatrix({
         0.0, -2.0,  0.0,
        -2.0,  8.0, -2.0,
         0.0, -2.0,  0.0
    });
    cvm->set_edgeMode(DrawingFilter::ConvolveMatrixEdgeMode::WRAP);
    cvm->set_preserveAlpha(false);

    // This is much slower in linearRGB which is the default
    auto rgb = Colors::Manager::get().find(Colors::Space::Type::RGB);
    cvm->setInterpolationSpace(rgb);

    EXPECT_PRIMITIVE_IS<PixelPatch::Method::ALPHA>(std::move(cvm),
                        "         O  "
                        "       .O   "
                        "      .o    "
                        "     .o     "
                        "    .o      "
                        "   .x       "
                        "    :-      "
                        "     :-     "
                        "      :-    "
                        "       :-   "
                        "        :-  "
                        "         :- ",
    // We clip a region of the result so we can see it in the pixel map
    Geom::Rect::from_xywh(35, 200, 25, 25));
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
