// SPDX-License-Identifier: GPL-2.0-or-later

#include "colors/manager.h"
#include "renderer/context.h"
#include "renderer/drawing-filters/image.h"
#include "drawing-testbase.h"
#include "surface-testbase.h"

using namespace Inkscape::Renderer;

TEST(DrawingImageTest, InlineRenderer)
{
    auto im = std::make_unique<DrawingFilter::Image>();
    im->set_output(1);
    im->set_item_box(Geom::Rect::from_xywh(0,0,600,600));
    im->set_render_function([](Context dc, DrawingOptions const &rc, Geom::IntRect const &area) {
        auto rgb = Colors::Manager::get().find(Colors::Space::Type::RGB);
        EXPECT_EQ(area, Geom::Rect::from_xywh(0,0,600,600));
        dc.rectangle(Geom::Rect::from_xywh(400, -100, 500, 500));
        dc.setSource(Colors::Color(rgb, {1.0, 0.0, 0.0, 1.0}));
        dc.fillPreserve();
        dc.setSource(Colors::Color(rgb, {0.0, 1.0, 0.0, 1.0}));
        dc.setLineWidth(30.0);
        dc.stroke();
    });

    // This is much slower in linearRGB which is the default
    auto rgb = Colors::Manager::get().find(Colors::Space::Type::RGB);
    im->setInterpolationSpace(rgb);

    EXPECT_PRIMITIVE_IS(std::move(im),
                        "          "
                        "          "
                        "   4      "
                        "  422     "
                        " 42222    "
                        "4222222   "
                        ".2222222  "
                        " .2222225 "
                        "  .222224 "
                        "   .2224  ");
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
