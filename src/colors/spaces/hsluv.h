// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2023 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_COLORS_SPACES_HSLUV_H
#define SEEN_COLORS_SPACES_HSLUV_H

#include <2geom/line.h>
#include <2geom/ray.h>

#include "luv.h"

#include <2geom/line.h>
#include <2geom/ray.h>

namespace Inkscape::Colors::Space {

class HSLuv : public XYZ
{
public:
    HSLuv(): XYZ(Type::HSLUV, 3, "HSLuv", "HSLuv", "color-selector-hsluv") {}
    ~HSLuv() override = default;

    bool isDirect() const override { return false; }

protected:
    friend class Inkscape::Colors::Color;

    void spaceToProfile(std::vector<double> &output) const override
    {
        HSLuv::toLuv(output);
        Luv::toXYZ(output);
    }
    void profileToSpace(std::vector<double> &output) const override
    {
        Luv::fromXYZ(output);
        HSLuv::fromLuv(output);
    }

public:
    static void toLuv(std::vector<double> &output);
    static void fromLuv(std::vector<double> &output);
    static std::array<Geom::Line, 6> get_bounds(double l);
};

} // namespace Inkscape::Colors::Space

#endif // SEEN_COLORS_SPACES_HSLUV_H
