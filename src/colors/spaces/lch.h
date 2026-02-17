// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2023 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_COLORS_SPACES_LCH_H
#define SEEN_COLORS_SPACES_LCH_H

#include "lab.h"

namespace Inkscape::Colors::Space {

class Lch : public Lab
{
public:
    Lch(): Lab(Type::LCH, 3, "Lch", "Lch", "color-selector-lch", true) {
        _svgNames.emplace_back("lch");
    }
    ~Lch() override = default;

    bool isDirect() const override { return false; }

protected:
    friend class Inkscape::Colors::Color;

    void spaceToProfile(std::vector<double> &output) const override
    {
        Lch::scaleUp(output);
        Lch::toLab(output);
        Lab::scaleDown(output);
    }
    void profileToSpace(std::vector<double> &output) const override
    {
        Lab::scaleUp(output);
        Lch::fromLab(output);
        Lch::scaleDown(output);
    }

    std::string toString(std::vector<double> const &values, bool opacity) const override;

public:
    class Parser : public Colors::Parser
    {
    public:
        Parser()
            : Colors::Parser("lch", Type::LCH)
        {}
        bool parse(std::istringstream &input, std::vector<double> &output) const override;
    };

    static void toLab(std::vector<double> &output);
    static void fromLab(std::vector<double> &output);

    static void scaleDown(std::vector<double> &in_out);
    static void scaleUp(std::vector<double> &in_out);
};

} // namespace Inkscape::Colors::Space

#endif // SEEN_COLORS_SPACES_LCH_H

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
