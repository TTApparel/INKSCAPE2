// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Mike Kowalski
 *
 * Copyright (C) 2025 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_COLORS_SPACES_OKHSV_H
#define SEEN_COLORS_SPACES_OKHSV_H

#include "oklab.h"

namespace Inkscape::Colors::Space {

class OkHsv : public RGB
{
public:
    OkHsv(): RGB(Type::OKHSV, 3, "OkHsv", "OkHsv", "color-selector-okhsv") {}
    ~OkHsv() override = default;

    bool isDirect() const override { return false; }

protected:
    void spaceToProfile(std::vector<double> &output) const override;
    void profileToSpace(std::vector<double> &output) const override;
};

} // namespace Inkscape::Colors::Space

#endif // SEEN_COLORS_SPACES_OKHSV_H
