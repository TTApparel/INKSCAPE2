// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Unit tests for the Linear RGB color space
 *
 * Copyright (C) 2023 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "colors/spaces/xyz.h"
#include "spaces-testbase.h"

namespace {

using Space::Type::XYZ;
using Space::Type::RGB;

// clang-format off
INSTANTIATE_TEST_SUITE_P(ColorsSpacesXYZ, fromString, testing::Values(
    _P(in, "color(xyz 0.1 1 0.5)", { 0.1, 1, 0.5 }, 0x00ff9bff)
));

INSTANTIATE_TEST_SUITE_P(ColorsSpacesXYZ, badColorString, testing::Values(
    "color(xyz", "color(xyz", "color(xyz 360"
));

INSTANTIATE_TEST_SUITE_P(ColorsSpacesXYZ, toString, testing::Values(
    _P(out, XYZ, { 0.3,   0.2,   0.8         }, "color(xyz 0.3 0.2 0.8)"),
    _P(out, XYZ, { 0.3,   0.8,   0.258       }, "color(xyz 0.3 0.8 0.258)"),
    _P(out, XYZ, { 1.0,   0.5,   0.004       }, "color(xyz 1 0.5 0.004)"),
    _P(out, XYZ, { 0,     1,     0.2,   0.8  }, "color(xyz 0 1 0.2 / 80%)", true),
    _P(out, XYZ, { 0,     1,     0.2,   0.8  }, "color(xyz 0 1 0.2)", false)
));

INSTANTIATE_TEST_SUITE_P(ColorsSpacesXYZ, convertColorSpace, testing::Values(
    // Example from w3c css-color-4 documentation
    _P(inb, XYZ, {0.217, 0.146, 0.594}, RGB, {0.463, 0.329, 0.804}),
    //_P(inb, XYZ, {0.217, 0.146, 0.594}, "Lab", {0.444, 0.644, 0.264}),
    // No conversion
    _P(inb, XYZ, {1.000, 0.400, 0.200}, XYZ, {1.000, 0.400, 0.200})
));

INSTANTIATE_TEST_SUITE_P(ColorsSpacesXYZ, normalize, testing::Values(
    _P(inb, XYZ, { 0.5,   0.5,   0.5,   0.5  }, XYZ, { 0.5,   0.5,   0.5,   0.5  }),
    _P(inb, XYZ, { 1.2,   1.2,   1.2,   1.2  }, XYZ, { 1.0,   1.0,   1.0,   1.0  }),
    _P(inb, XYZ, {-0.2,  -0.2,  -0.2,  -0.2  }, XYZ, { 0.0,   0.0,   0.0,   0.0  }),
    _P(inb, XYZ, { 0.0,   0.0,   0.0,   0.0  }, XYZ, { 0.0,   0.0,   0.0,   0.0  }),
    _P(inb, XYZ, { 1.0,   1.0,   1.0,   1.0  }, XYZ, { 1.0,   1.0,   1.0,   1.0  })
));
// clang-format on

TEST(ColorsSpacesXYZ, randomConversion)
{
    // Isolate conversion functions
    EXPECT_TRUE(RandomPassFunc(Space::XYZ::fromLinearRGB, Space::XYZ::toLinearRGB, 1000));

    // Full stack conversion
    EXPECT_TRUE(RandomPassthrough(XYZ, RGB, 1000));
}

TEST(ColorsSpacesXYZ, components)
{
    auto c = Manager::get().find(XYZ)->getComponents();
    ASSERT_EQ(c.size(), 3);
    EXPECT_EQ(c[0].id, "x");
    EXPECT_EQ(c[1].id, "y");
    EXPECT_EQ(c[2].id, "z");
}

} // namespace

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
