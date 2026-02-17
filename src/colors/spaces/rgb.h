// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2023 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_COLORS_SPACES_RGB_H
#define SEEN_COLORS_SPACES_RGB_H

#include "base.h"

namespace Inkscape::Colors::Space {

class RGB : public AnySpace
{
public:
    RGB(): AnySpace(Type::RGB, 3, "RGB", "RGB", "color-selector-rgb") {
        _svgNames.emplace_back("sRGB");
    }
    ~RGB() override = default;

    bool isDirect() const override { return true; }
    std::shared_ptr<Colors::CMS::Profile> const getProfile() const override;

protected:
    RGB(Type type, int components, std::string name, std::string shortName, std::string icon, bool spaceIsUnbounded = false);

    friend class Inkscape::Colors::Color;

    std::string toString(std::vector<double> const &values, bool opacity = true) const override;

public:
    class Parser : public LegacyParser
    {
    public:
        Parser(bool alpha)
            : LegacyParser("rgb", Type::RGB, alpha)
        {}
        bool parse(std::istringstream &input, std::vector<double> &output) const override;
    };
};

} // namespace Inkscape::Colors::Space

#endif // SEEN_COLORS_SPACES_RGB_H
