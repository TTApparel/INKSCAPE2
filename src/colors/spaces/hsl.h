// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2023 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_COLORS_SPACES_HSL_H
#define SEEN_COLORS_SPACES_HSL_H

#include "rgb.h"

namespace Inkscape::Colors::Space {

class HSL : public RGB
{
public:
    HSL(): RGB(Type::HSL, 3, "HSL", "HSL", "color-selector-hsx") {
        _svgNames.emplace_back("hsl");
    }
    ~HSL() override = default;

    bool isDirect() const override { return false; }

protected:
    friend class Inkscape::Colors::Color;

    std::string toString(std::vector<double> const &values, bool opacity = true) const override;

    void spaceToProfile(std::vector<double> &output) const override;
    void profileToSpace(std::vector<double> &output) const override;

public:
    class Parser : public HueParser
    {
    public:
        Parser(bool alpha)
	     : HueParser("hsl", Type::HSL, alpha, 100.0)
        {}
    };
};

} // namespace Inkscape::Colors::Space

#endif // SEEN_COLORS_SPACES_HSL_H
