// SPDX-License-Identifier: GPL-2.0-or-later

#include "renderer/drawing-filters/component-transfer.h"
#include "drawing-testbase.h"
#include "surface-testbase.h"

using namespace Inkscape::Renderer;

TEST(DrawingComponentTransferTest, MatrixTable)
{
    auto ct = std::make_unique<DrawingFilter::ComponentTransfer>();
    ct->set_output(1);
    // Test the rainbow
    ct->set_input(DrawingFilter::SLOT_BACKGROUND_IMAGE);
    ct->tableValues[0] = {0, 0, 1, 1};
    ct->tableValues[1] = {1, 1, 0, 0};
    ct->tableValues[2] = {0, 1, 1, 0};
    ct->tableValues[3] = {0, 0, 0, 1};
    ct->type[0] = DrawingFilter::ComponentTransferType::TABLE;
    ct->type[1] = DrawingFilter::ComponentTransferType::TABLE;
    ct->type[2] = DrawingFilter::ComponentTransferType::TABLE;
    ct->type[3] = DrawingFilter::ComponentTransferType::TABLE;

    // This is much slower in linearRGB which is the default
    auto rgb = Colors::Manager::get().find(Colors::Space::Type::RGB);
    ct->setInterpolationSpace(rgb);

    EXPECT_PRIMITIVE_IS(std::move(ct),
                        "2222222222"
                        "9999999999"
                        "4444444444"
                        ".........."
                        "PPPPPPPPPP"
                        ".........."
                        "1111111111"
                        "1111111111"
                        "2222222222"
                        "6666666666");
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
