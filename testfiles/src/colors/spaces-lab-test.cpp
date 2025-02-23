// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Unit tests for the Lab color space
 *
 * Copyright (C) 2023 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "colors/spaces/lab.h"
#include "spaces-testbase.h"

namespace {

using Space::Type::LAB;
using Space::Type::RGB;

// clang-format off
INSTANTIATE_TEST_SUITE_P(ColorsSpacesLAB, fromString, testing::Values(
    _P(in, "lab(50% -20 0.5)",     { 0.5,  0.42, 0.502     }, 0x4c8175ff),
    _P(in, "lab(75 -125 125)",     { 0.75, 0.0,  1.0       }, 0x00e300ff),
    _P(in, "lab(0 0 0)",           { 0.0,  0.5,  0.5       }, 0x000000ff),
    _P(in, "lab(20% 20 20 / 20%)", { 0.2,  0.58, 0.58, 0.2 }, 0x51231333)
));

INSTANTIATE_TEST_SUITE_P(ColorsSpacesLAB, badColorString, testing::Values(
    "lab", "lab(", "lab(100"
));

INSTANTIATE_TEST_SUITE_P(ColorsSpacesLAB, toString, testing::Values(
    _P(out, LAB, { 0.3,   0.2,   0.8         }, "lab(30 -75 75)"),
    _P(out, LAB, { 0.3,   0.8,   0.258       }, "lab(30 75 -60.5)"),
    _P(out, LAB, { 1.0,   0.5,   0.004       }, "lab(100 0 -124)"),
    _P(out, LAB, { 0,     1,     0.2,   0.8  }, "lab(0 125 -75 / 80%)", true),
    _P(out, LAB, { 0,     1,     0.2,   0.8  }, "lab(0 125 -75)", false)
));

INSTANTIATE_TEST_SUITE_P(ColorsSpacesLAB, convertColorSpace, testing::Values(
    // Example from w3c css-color-4 documentation
    _P(inb, LAB, {0.462, 0.309, 0.694}, RGB, {0.097, 0.499, 0.006}),
    // No conversion
    _P(inb, LAB, {1.000, 0.400, 0.200}, LAB, {1.000, 0.400, 0.200})
));

INSTANTIATE_TEST_SUITE_P(ColorsSpacesLAB, normalize, testing::Values(
    _P(inb, LAB, { 0.5,   0.5,   0.5,   0.5  }, LAB, { 0.5,   0.5,   0.5,   0.5  }),
    _P(inb, LAB, { 1.2,   1.2,   1.2,   1.2  }, LAB, { 1.0,   1.0,   1.0,   1.0  }),
    _P(inb, LAB, {-0.2,  -0.2,  -0.2,  -0.2  }, LAB, { 0.0,   0.0,   0.0,   0.0  }),
    _P(inb, LAB, { 0.0,   0.0,   0.0,   0.0  }, LAB, { 0.0,   0.0,   0.0,   0.0  }),
    _P(inb, LAB, { 1.0,   1.0,   1.0,   1.0  }, LAB, { 1.0,   1.0,   1.0,   1.0  })
));
// clang-format on

TEST(ColorsSpacesLAB, randomConversion)
{
    // Isolate conversion functions
    EXPECT_TRUE(RandomPassFunc(Space::Lab::fromXYZ, Space::Lab::toXYZ, 1000));

    // Full stack conversion
    EXPECT_TRUE(RandomPassthrough(LAB, RGB, 1000));
}

TEST(ColorsSpacesLAB, components)
{
    auto c = Manager::get().find(LAB)->getComponents();
    ASSERT_EQ(c.size(), 3);
    EXPECT_EQ(c[0].id, "l");
    EXPECT_EQ(c[1].id, "a");
    EXPECT_EQ(c[2].id, "b");
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
