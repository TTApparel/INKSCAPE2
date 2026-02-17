// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2023 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_COLORS_SPACES_DEVICECMYK_H
#define SEEN_COLORS_SPACES_DEVICECMYK_H

#include "rgb.h"

namespace Inkscape::Colors::Space {

/**
 * This sRGB based DeviceCMYK space is uncalibrated and fixed to the sRGB icc profile.
 */
class DeviceCMYK : public RGB
{
public:
    DeviceCMYK(): RGB(Type::CMYK, 4, "DeviceCMYK", "CMYK", "color-selector-cmyk") {
        _svgNames.emplace_back("device-cmyk");
    }
    ~DeviceCMYK() override = default;

    bool isDirect() const override { return false; }

protected:
    friend class Inkscape::Colors::Color;

    void spaceToProfile(std::vector<double> &output) const override;
    void profileToSpace(std::vector<double> &output) const override;

    std::string toString(std::vector<double> const &values, bool opacity = true) const override;
    bool overInk(std::vector<double> const &input) const override;
};

} // namespace Inkscape::Colors::Space

#endif // SEEN_COLORS_SPACES_DEVICECMYK_H
