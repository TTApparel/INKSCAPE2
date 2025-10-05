// SPDX-License-Identifier: GPL-2.0-or-later

#include "renderer/pixel-filters/component-transfer.h"
#include "pixel-access-testbase.h"

using namespace Inkscape::Renderer::PixelFilter;

TEST(PixelFilterComponentTest, ComponentTransfer)
{
    // Default transfer is Identity
    EXPECT_TRUE(FilterColors<3>(ComponentTransfer({}), {1.0, 0.0, 1.0, 0.5}, {1.0, 0.0, 1.0, 0.5}));
}

TEST(PixelFilterComponentTest, ComponentTransferTable)
{
    std::vector<TransferFunction> tfs = {
        TransferFunction({0, 0, 1, 1}, false),
        TransferFunction({1, 1, 0, 0}, false),
        TransferFunction({0, 1, 1, 0}, false),
    };
    // NOTE: Input colors are in sRGB, not in linearRGB
    EXPECT_TRUE(FilterColors<3>(ComponentTransfer(tfs), {1.0, 1.0, 0.0, 1.0}, {1.0, 0.0, 0.0, 1.0}));
    EXPECT_TRUE(FilterColors<3>(ComponentTransfer(tfs), {1.0, 0.4, 0.0, 1.0}, {1.0, 0.2, 0.0, 1.0}));
    EXPECT_TRUE(FilterColors<3>(ComponentTransfer(tfs), {1.0, 0.0, 0.0, 1.0}, {1.0, 1.0, 0.0, 1.0}));
    EXPECT_TRUE(FilterColors<3>(ComponentTransfer(tfs), {0.0, 0.0, 0.0, 1.0}, {0.0, 1.0, 0.0, 1.0}));
    EXPECT_TRUE(FilterColors<3>(ComponentTransfer(tfs), {0.0, 0.0, 0.8, 1.0}, {0.0, 1.0, 0.7, 1.0}));
}

TEST(PixelFilterComponentTest, ComponentTransferDiscrete)
{
    std::vector<TransferFunction> tfs = {
        TransferFunction({0, 0, 1, 1}, true),
        TransferFunction({1, 1, 0, 0}, true),
        TransferFunction({0, 1, 1, 0}, true),
    };
    EXPECT_TRUE(FilterColors<3>(ComponentTransfer(tfs), {1.0, 1.0, 0.0, 1.0}, {1.0, 0.0, 0.0, 1.0}));
    EXPECT_TRUE(FilterColors<3>(ComponentTransfer(tfs), {1.0, 1.0, 0.0, 1.0}, {1.0, 0.2, 0.0, 1.0}));
    EXPECT_TRUE(FilterColors<3>(ComponentTransfer(tfs), {1.0, 0.0, 0.0, 1.0}, {1.0, 1.0, 0.0, 1.0}));
    EXPECT_TRUE(FilterColors<3>(ComponentTransfer(tfs), {0.0, 0.0, 0.0, 1.0}, {0.0, 1.0, 0.0, 1.0}));
    EXPECT_TRUE(FilterColors<3>(ComponentTransfer(tfs), {0.0, 0.0, 1.0, 1.0}, {0.0, 1.0, 0.7, 1.0}));
}

TEST(PixelFilterComponentTest, ComponentTransferLinear)
{
    std::vector<TransferFunction> tfs = {
        TransferFunction(0.5, 0.0),
        TransferFunction(0.5, 0.25),
        TransferFunction(0.5, 0.5),
    };
    EXPECT_TRUE(FilterColors<3>(ComponentTransfer(tfs), {0.5, 0.25, 0.5, 1.0}, {1.0, 0.0, 0.0, 1.0}));
    EXPECT_TRUE(FilterColors<3>(ComponentTransfer(tfs), {0.5, 0.35, 0.5, 1.0}, {1.0, 0.2, 0.0, 1.0}));
    EXPECT_TRUE(FilterColors<3>(ComponentTransfer(tfs), {0.5, 0.75, 0.5, 1.0}, {1.0, 1.0, 0.0, 1.0}));
    EXPECT_TRUE(FilterColors<3>(ComponentTransfer(tfs), {0.0, 0.75, 0.5, 1.0}, {0.0, 1.0, 0.0, 1.0}));
    EXPECT_TRUE(FilterColors<3>(ComponentTransfer(tfs), {0.0, 0.75, 0.85, 1.0}, {0.0, 1.0, 0.7, 1.0}));
}

TEST(PixelFilterComponentTest, ComponentTransferGamma)
{
    std::vector<TransferFunction> tfs = {
        TransferFunction(4.0, 7.0, 0.0),
        TransferFunction(4.0, 4.0, 0.0),
        TransferFunction(4.0, 1.0, 0.0),
    };
    EXPECT_TRUE(FilterColors<3>(ComponentTransfer(tfs), {1.0, 0.0, 0.0, 1.0}, {1.0, 0.0, 0.0, 1.0}));
    EXPECT_TRUE(FilterColors<3>(ComponentTransfer(tfs), {1.0, 0.25, 0.0, 1.0}, {1.0, 0.5, 0.0, 1.0}));
    EXPECT_TRUE(FilterColors<3>(ComponentTransfer(tfs), {1.0, 1.0, 0.0, 1.0}, {1.0, 1.0, 0.0, 1.0}));
    EXPECT_TRUE(FilterColors<3>(ComponentTransfer(tfs), {0.0, 1.0, 0.0, 1.0}, {0.0, 1.0, 0.0, 1.0}));
    EXPECT_TRUE(FilterColors<3>(ComponentTransfer(tfs), {0.0, 1.0, 1.0, 1.0}, {0.0, 1.0, 0.5, 1.0}));
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
