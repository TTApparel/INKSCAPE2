// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 *//*
 * Authors:
 *   2023 Martin Owens
 *
 * Copyright (C) 2023 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "lab.h"

#include <cmath>

#include "colors/color.h"
#include "colors/printer.h"

namespace Inkscape::Colors::Space {

constexpr double LUMA_SCALE = 100;

// CSS Actual values are scaled -128 -> 127
constexpr double MIN_SCALE = -128;
constexpr double MAX_SCALE = 127;

Lab::Lab(Type type, int components, std::string name, std::string shortName, std::string icon, bool spaceIsUnbounded):
    AnySpace(type, components, std::move(name), std::move(shortName), std::move(icon), spaceIsUnbounded) {
}

/**
 * Return the RGB color profile, this is static for all RGB sub-types
 */
std::shared_ptr<Inkscape::Colors::CMS::Profile> const Lab::getProfile() const
{
    static std::shared_ptr<Colors::CMS::Profile> lab_profile = Colors::CMS::Profile::create_lab();
    return lab_profile;
}

/**
 * Changes the values from 0..1, to typical lab scaling used in calculations.
 */
void Lab::scaleUp(std::vector<double> &in_out)
{
    in_out[0] = SCALE_UP(in_out[0], 0, LUMA_SCALE);
    in_out[1] = SCALE_UP(in_out[1], MIN_SCALE, MAX_SCALE);
    in_out[2] = SCALE_UP(in_out[2], MIN_SCALE, MAX_SCALE);
}

/**
 * Changes the values from typical lab scaling (see above) to values 0..1.
 */
void Lab::scaleDown(std::vector<double> &in_out)
{
    in_out[0] = SCALE_DOWN(in_out[0], 0, LUMA_SCALE);
    in_out[1] = SCALE_DOWN(in_out[1], MIN_SCALE, MAX_SCALE);
    in_out[2] = SCALE_DOWN(in_out[2], MIN_SCALE, MAX_SCALE);
}

/**
 * Print the Lab color to a CSS string.
 *
 * @arg values - A vector of doubles for each channel in the Lab space
 * @arg opacity - True if the opacity should be included in the output.
 */
std::string Lab::toString(std::vector<double> const &values, bool opacity) const
{
    auto os = CssFuncPrinter(3, "lab");

    os << values[0] * LUMA_SCALE                             // Luminance
       << SCALE_UP(values[1], MIN_SCALE, MAX_SCALE)  // Chroma A
       << SCALE_UP(values[2], MIN_SCALE, MAX_SCALE); // Chroma B

    if (opacity && values.size() == 4)
        os << values[3]; // Optional opacity

    return os;
}

bool Lab::Parser::parse(std::istringstream &ss, std::vector<double> &output) const
{
    // CSS Color Module 4 defines 100% as 125 for A/B in lab
    static double CSS_PERCENT_SCALE = (100.0 / 125.0);

    bool end = false;
    if (append_css_value(ss, output, end, ',', LUMA_SCALE)                  // Lightness
        && append_css_value(ss, output, end, ',', 1.0, CSS_PERCENT_SCALE) // Chroma-A
        && append_css_value(ss, output, end, '/', 1.0, CSS_PERCENT_SCALE) // Chroma-B
        && (append_css_value(ss, output, end) || true)                      // Optional opacity
        && end) {
        // The A and B portions are between -128 and +127 which is hard for append_css_value to convert
        output[1] = SCALE_DOWN(output[1], MIN_SCALE, MAX_SCALE);
        output[2] = SCALE_DOWN(output[2], MIN_SCALE, MAX_SCALE);
        return true;
    }
    return false;
}

}; // namespace Inkscape::Colors::Space
