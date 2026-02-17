// SPDX-License-Identifier: GPL-2.0-or-later

#include "renderer/drawing-filters/morphology.h"
#include "drawing-testbase.h"
#include "surface-testbase.h"

using namespace Inkscape::Renderer;

TEST(DrawingMergeTest, SimpleMerge)
{
    auto mg = std::make_unique<DrawingFilter::Morphology>();
    mg->set_output(1);
    mg->set_operator(DrawingFilter::MorphologyOperator::ERODE);
    mg->set_xradius(0.06);
    mg->set_yradius(0.02);

    // This is much slower in linearRGB which is the default
    auto rgb = Colors::Manager::get().find(Colors::Space::Type::RGB);
    mg->setInterpolationSpace(rgb);

    // The result is squashed offset 45 degrees
    EXPECT_PRIMITIVE_IS(std::move(mg),
                        "          "
                        "          "
                        "          "
                        "   48     "
                        "   888    "
                        "    888   "
                        "     884  "
                        "      4   "
                        "          "
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
