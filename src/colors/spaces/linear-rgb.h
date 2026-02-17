// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2023 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_COLORS_SPACES_LINEARRGB_H
#define SEEN_COLORS_SPACES_LINEARRGB_H

#include "base.h"

namespace Inkscape::Colors::Space {

class LinearRGB : public AnySpace
{
public:
    LinearRGB(): AnySpace(Type::linearRGB, 3, "linearRGB", "linearRGB", "color-selector-linear-rgb") {
        _svgNames.emplace_back("linearRGB");
        _svgNames.emplace_back("srgb-linear");
        _intent = RenderingIntent::RELATIVE_COLORIMETRIC;
        _intent_priority = 10;
    }
    ~LinearRGB() override = default;

    bool isDirect() const override { return true; }
    std::shared_ptr<Colors::CMS::Profile> const getProfile() const override;

protected:
    friend class Inkscape::Colors::Color;

    std::string toString(std::vector<double> const &values, bool opacity = true) const override;

public:
    static void toRGB(std::vector<double> &output);
    static void fromRGB(std::vector<double> &output);
};

} // namespace Inkscape::Colors::Space

#endif // SEEN_COLORS_SPACES_LINEARRGB_H
