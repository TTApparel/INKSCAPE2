// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Unit tests for color objects.
 *
 * Copyright (C) 2023 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "colors/manager.h"

#include <gtest/gtest.h>

#include "colors/cms/system.h"
#include "colors/color.h"
#include "colors/spaces/base.h"
#include "colors/spaces/cms.h"
#include "colors/spaces/rgb.h"
#include "colors/spaces/components.h"
#include "colors/spaces/enum.h"
#include "document.h"
#include "inkscape.h"
#include "object/color-profile.h"

using namespace Inkscape;
using namespace Inkscape::Colors;

namespace {

class TestManager : public Manager
{
public:
    TestManager() = default;
    ~TestManager() = default;

    std::shared_ptr<Space::AnySpace> testAddSpace(Space::AnySpace *space)
    {
        return addSpace(space);
    }

    bool testRemoveSpace(std::shared_ptr<Space::AnySpace> space)
    {
        return removeSpace(std::move(space));
    }

};

TEST(ColorManagerTest, spaceComponents)
{
    auto &cm = Manager::get();

    ASSERT_TRUE(cm.find(Space::Type::RGB));
    auto comp = cm.find(Space::Type::RGB)->getComponents();
    ASSERT_EQ(comp.size(), 3);
    ASSERT_EQ(comp[0].name, "_R");
    ASSERT_EQ(comp[1].name, "_G");
    ASSERT_EQ(comp[2].name, "_B");

    ASSERT_TRUE(cm.find(Space::Type::HSL));
    comp = cm.find(Space::Type::HSL)->getComponents(true);
    ASSERT_EQ(comp.size(), 4);
    ASSERT_EQ(comp[0].name, "_H");
    ASSERT_EQ(comp[1].name, "_S");
    ASSERT_EQ(comp[2].name, "_L");
    ASSERT_EQ(comp[3].name, "_A");

    ASSERT_TRUE(cm.find(Space::Type::CMYK));
    comp = cm.find(Space::Type::CMYK)->getComponents(false);
    ASSERT_EQ(comp.size(), 4);
    ASSERT_EQ(comp[0].name, "_C");
    ASSERT_EQ(comp[1].name, "_M");
    ASSERT_EQ(comp[2].name, "_Y");
    ASSERT_EQ(comp[3].name, "_K");
}

TEST(ColorManagerTest, addAndRemoveSpaces)
{
    auto cm = TestManager();

    auto rgb = cm.find(Space::Type::RGB);
    ASSERT_TRUE(rgb);

    EXPECT_THROW(cm.testAddSpace(rgb.get()), ColorError);

    ASSERT_TRUE(cm.testRemoveSpace(rgb));
    ASSERT_FALSE(cm.testRemoveSpace(rgb));
    ASSERT_FALSE(cm.find(Space::Type::RGB));

    cm.testAddSpace(new Space::RGB());
    ASSERT_TRUE(cm.find(Space::Type::RGB));
}

TEST(ColorManagerTest, getSpaces)
{
    auto cm = TestManager();

    auto none = cm.spaces(Space::Traits::None);
    ASSERT_EQ(none.size(), 0);

    auto internal = cm.spaces(Space::Traits::Internal);
    ASSERT_GT(internal.size(), 0);
    ASSERT_EQ(internal[0]->getComponents().traits() & Space::Traits::Internal, Space::Traits::Internal);

    auto pickers = cm.spaces(Space::Traits::Picker);
    ASSERT_GT(pickers.size(), 0);
    ASSERT_EQ(pickers[0]->getComponents().traits() & Space::Traits::Picker, Space::Traits::Picker);

    auto mix = cm.spaces(Space::Traits::Picker | Space::Traits::Internal);
    ASSERT_EQ(mix.size(), internal.size() + pickers.size());
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
