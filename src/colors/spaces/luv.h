// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2023 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_COLORS_SPACES_LUV_H
#define SEEN_COLORS_SPACES_LUV_H

#include "xyz.h"

namespace Inkscape::Colors::Space {

// CIE LUV constants
constexpr double KAPPA = 903.29629629629629629630;
constexpr double EPSILON = 0.00885645167903563082;

class Luv : public XYZ
{
public:
    Luv(): XYZ(Type::LUV, 3, "Luv", "Luv", "color-selector-luv") {}
    ~Luv() override = default;

    bool isDirect() const override { return false; }

protected:
    friend class Inkscape::Colors::Color;

    void spaceToProfile(std::vector<double> &output) const override
    {
        Luv::scaleUp(output);
        Luv::toXYZ(output);
    }
    void profileToSpace(std::vector<double> &output) const override
    {
        Luv::fromXYZ(output);
        Luv::scaleDown(output);
    }

public:
    static void toXYZ(std::vector<double> &output);
    static void fromXYZ(std::vector<double> &output);

    static void scaleDown(std::vector<double> &in_out);
    static void scaleUp(std::vector<double> &in_out);

    static std::vector<double> fromCoordinates(std::vector<double> const &in);
    static std::vector<double> toCoordinates(std::vector<double> const &in);
};

} // namespace Inkscape::Colors::Space

#endif // SEEN_COLORS_SPACES_LUV_H
