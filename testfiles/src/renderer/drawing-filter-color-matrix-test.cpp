// SPDX-License-Identifier: GPL-2.0-or-later

#include "renderer/drawing-filters/color-matrix.h"
#include "drawing-testbase.h"
#include "surface-testbase.h"

using namespace Inkscape::Renderer;

TEST(DrawingColorMatrixTest, HueRotate)
{
    auto cm = std::make_unique<DrawingFilter::ColorMatrix>();
    cm->set_output(1);
    cm->set_type(DrawingFilter::ColorMatrixType::HUEROTATE);
    cm->set_value(180.0);

    // This is much slower in linearRGB which is the default
    auto rgb = Colors::Manager::get().find(Colors::Space::Type::RGB);
    cm->setInterpolationSpace(rgb);

    EXPECT_PRIMITIVE_IS(std::move(cm),
                        "          "
                        "     R    "
                        "    RRR   "
                        "   RRRRR  "
                        "  RRRRRRR "
                        " RRRRRRRR "
                        "  RRRRRRA "
                        "   RRRRA  "
                        "    RRA   "
                        "          ");
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
