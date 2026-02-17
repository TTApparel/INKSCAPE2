// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2023 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_COLORS_SPACES_OKLCH_H
#define SEEN_COLORS_SPACES_OKLCH_H

#include "oklab.h"

namespace Inkscape::Colors::Space {

class OkLch : public RGB
{
public:
    OkLch(): RGB(Type::OKLCH, 3, "OkLch", "OkLch", "color-selector-oklch") {
        _svgNames.emplace_back("oklch");
    }
    ~OkLch() override = default;

    bool isDirect() const override { return false; }

protected:
    friend class Inkscape::Colors::Color;

    void spaceToProfile(std::vector<double> &output) const override
    {
        OkLch::toOkLab(output);
        OkLab::toLinearRGB(output);
        LinearRGB::toRGB(output);
    }
    void profileToSpace(std::vector<double> &output) const override
    {
        LinearRGB::fromRGB(output);
        OkLab::fromLinearRGB(output);
        OkLch::fromOkLab(output);
    }

    std::string toString(std::vector<double> const &values, bool opacity) const override;

public:
    class Parser : public Colors::Parser
    {
    public:
        Parser()
            : Colors::Parser("oklch", Type::OKLCH)
        {}
        bool parse(std::istringstream &input, std::vector<double> &output) const override;
    };

    static void toOkLab(std::vector<double> &output);
    static void fromOkLab(std::vector<double> &output);
    static double max_chroma(double l, double h);
};

uint8_t const *render_hue_scale(double s, double l, std::array<uint8_t, 4 * 1024> *map);
uint8_t const *render_saturation_scale(double h, double l, std::array<uint8_t, 4 * 1024> *map);
uint8_t const *render_lightness_scale(double h, double s, std::array<uint8_t, 4 * 1024> *map);

} // namespace Inkscape::Colors::Space

#endif // SEEN_COLORS_SPACES_OKLCH_H
