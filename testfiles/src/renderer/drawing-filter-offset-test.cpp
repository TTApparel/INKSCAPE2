// SPDX-License-Identifier: GPL-2.0-or-later

#include "renderer/drawing-filters/offset.h"
#include "drawing-testbase.h"
#include "surface-testbase.h"

using namespace Inkscape::Renderer;

TEST(DrawingOffsetTest, SimpleOffset)
{
    auto of = std::make_unique<DrawingFilter::Offset>();
    of->set_output(1);
    of->set_dx(0.1);
    of->set_dy(0);

    // This is much slower in linearRGB which is the default
    auto rgb = Colors::Manager::get().find(Colors::Space::Type::RGB);
    of->setInterpolationSpace(rgb);

    EXPECT_PRIMITIVE_IS(std::move(of),
                        "          "
                        "        8 "
                        "       888"
                        "      8888"
                        "     88888"
                        "    888888"
                        "     88888"
                        "      8888"
                        "       884"
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
