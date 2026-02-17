// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2023 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_COLORS_SPACES_LAB_H
#define SEEN_COLORS_SPACES_LAB_H

#include "base.h"

namespace Inkscape::Colors::Space {

class Lab : public AnySpace
{
public:
    Lab(): AnySpace(Type::LAB, 3, "Lab", "Lab", "color-selector-lab", true) {
        _svgNames.emplace_back("lab");
        _intent = RenderingIntent::ABSOLUTE_COLORIMETRIC;
        _intent_priority = 10;
    }
    ~Lab() override = default;

    bool isDirect() const override { return true; }
    std::shared_ptr<Inkscape::Colors::CMS::Profile> const getProfile() const override;

protected:
    friend class Inkscape::Colors::Color;

    Lab(Type type, int components, std::string name, std::string shortName, std::string icon, bool spaceIsUnbounded = false);

    std::string toString(std::vector<double> const &values, bool opacity) const override;

public:
    class Parser : public Colors::Parser
    {
    public:
        Parser()
            : Colors::Parser("lab", Type::LAB)
        {}
        bool parse(std::istringstream &input, std::vector<double> &output) const override;
    };

    static void scaleDown(std::vector<double> &in_out);
    static void scaleUp(std::vector<double> &in_out);
};

} // namespace Inkscape::Colors::Space

#endif // SEEN_COLORS_SPACES_LAB_H

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
