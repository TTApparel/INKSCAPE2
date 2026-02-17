// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 *//*
 * Authors: see git history
 *
 * Copyright (C) 2023 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "alpha.h"

namespace Inkscape::Colors::Space {

/**
 * Return the Gray color profile
 */
std::shared_ptr<Inkscape::Colors::CMS::Profile> const Alpha::getProfile() const
{
    static std::shared_ptr<Colors::CMS::Profile> gray_profile;
    if (!gray_profile) {
        gray_profile = Colors::CMS::Profile::create_gray();
    }   
    return gray_profile;
}

}; // namespace Inkscape::Colors::Space
