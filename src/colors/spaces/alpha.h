// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2023 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_COLORS_SPACES_ALPHA_H
#define SEEN_COLORS_SPACES_ALPHA_H

#include "base.h"

namespace Inkscape::Colors::Space {

/**
 * This space gives incorrect results for a regular color, because it's not meant to
 * be used in color pickers or store data in a color field. Instead it's purely used
 * for converting a surface of pixels into a luminosity mask while rendering.
 */
class Alpha : public AnySpace
{
public:
    Alpha(): AnySpace(Type::Alpha, 0, "Alpha", "Alpha", "") {}

    bool isDirect() const override { return true; }
    std::shared_ptr<Colors::CMS::Profile> const getProfile() const override;

    std::string toString(std::vector<double> const &values, bool opacity) const override
    {
        return "";
    }
};

} // namespace Inkscape::Colors::Space

#endif // SEEN_COLORS_SPACES_ALPHA_H
