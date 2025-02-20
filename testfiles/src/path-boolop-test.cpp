// SPDX-License-Identifier: GPL-2.0-or-later
#include <gtest/gtest.h>
#include <2geom/svg-path-writer.h>
#include "path/path-boolop.h"
#include "svg/svg.h"

class PathBoolopTest : public ::testing::Test
{
public:
    Geom::PathVector const rectangle_bigger = sp_svg_read_pathv("M 0,0 L 0,2 L 2,2 L 2,0 z");
    Geom::PathVector const rectangle_smaller = sp_svg_read_pathv("M 0.5,0.5 L 0.5,1.5 L 1.5,1.5 L 1.5,0.5 z");
    Geom::PathVector const rectangle_outside = sp_svg_read_pathv("M 0,1.5 L 0.5,1.5 L 0.5,2.5 L 0,2.5 z");
    Geom::PathVector const reference_union = sp_svg_read_pathv("M 0,0 L 0,1.5 L 0,2 L 0,2.5 L 0.5,2.5 L 0.5,2 L 2,2 L 2,0 L 0,0 z");
    Geom::PathVector const empty = sp_svg_read_pathv("");

    static void comparePaths(Geom::PathVector const &result, Geom::PathVector const &reference)
    {
        Geom::SVGPathWriter wr;
        wr.feed(result);
        auto const resultD = wr.str();
        wr.clear();
        wr.feed(reference);
        auto const referenceD = wr.str();
        EXPECT_EQ(resultD, referenceD);
        EXPECT_EQ(result, reference);
    }
};

TEST_F(PathBoolopTest, UnionOutside) {
    // test that the union of two objects where one is outside the other results in a new larger shape
    auto pathv = sp_pathvector_boolop(rectangle_bigger, rectangle_outside, bool_op_union, fill_oddEven, fill_oddEven);
    comparePaths(pathv, reference_union);
}

TEST_F(PathBoolopTest, UnionOutsideSwap) {
    // test that the union of two objects where one is outside the other results in a new larger shape, even when the order is reversed
    auto pathv = sp_pathvector_boolop(rectangle_outside, rectangle_bigger, bool_op_union, fill_oddEven, fill_oddEven);
    comparePaths(pathv, reference_union);
}

TEST_F(PathBoolopTest, UnionInside) {
    // test that the union of two objects where one is completely inside the other is the larger shape
    auto pathv = sp_pathvector_boolop(rectangle_bigger, rectangle_smaller, bool_op_union, fill_oddEven, fill_oddEven);
    comparePaths(pathv, rectangle_bigger);
}

TEST_F(PathBoolopTest, UnionInsideSwap) {
    // test that the union of two objects where one is completely inside the other is the larger shape, even when the order is swapped
    auto pathv = sp_pathvector_boolop(rectangle_smaller, rectangle_bigger, bool_op_union, fill_oddEven, fill_oddEven);
    comparePaths(pathv, rectangle_bigger);
}

TEST_F(PathBoolopTest, IntersectionInside) {
    // test that the intersection of two objects where one is completely inside the other is the smaller shape
    auto pathv = sp_pathvector_boolop(rectangle_bigger, rectangle_smaller, bool_op_inters, fill_oddEven, fill_oddEven);
    comparePaths(pathv, rectangle_smaller);
}

TEST_F(PathBoolopTest, DifferenceInside) {
    // test that the difference of two objects where one is completely inside the other is an empty path
    auto pathv = sp_pathvector_boolop(rectangle_bigger, rectangle_smaller, bool_op_diff, fill_oddEven, fill_oddEven);
    comparePaths(pathv, empty);
}

TEST_F(PathBoolopTest, DifferenceOutside) {
    // test that the difference of two objects where one is completely outside the other is multiple shapes
    auto pathv = sp_pathvector_boolop(rectangle_smaller, rectangle_bigger, bool_op_diff, fill_oddEven, fill_oddEven);

    auto both_paths = rectangle_bigger;
    for (auto const &path : rectangle_smaller) {
        both_paths.push_back(path.reversed());
    }

    comparePaths(pathv, both_paths);
}
