// SPDX-License-Identifier: GPL-2.0-or-later

#include "renderer/drawing-filters/merge.h"
#include "drawing-testbase.h"
#include "surface-testbase.h"

using namespace Inkscape::Renderer;

TEST(DrawingMergeTest, SimpleMerge)
{
    auto mg = std::make_unique<DrawingFilter::Merge>();
    mg->set_output(1);
    mg->set_input(0, DrawingFilter::SLOT_BACKGROUND_IMAGE);
    mg->set_input(1, DrawingFilter::SLOT_SOURCE_IMAGE);

    // This is much slower in linearRGB which is the default
    auto rgb = Colors::Manager::get().find(Colors::Space::Type::RGB);
    mg->setInterpolationSpace(rgb);

    EXPECT_PRIMITIVE_IS(std::move(mg),
                        "RRRRRRRRRR"
                        "QQQQQHQQQQ"
                        "PPPP88HPPP"
                        "XXX8888HXX"
                        "HH8888888H"
                        "8888888888"
                        "9988888899"
                        ":::9888:::"
                        "::::98::::"
                        "2222222222"
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
