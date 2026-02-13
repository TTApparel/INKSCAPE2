// SPDX-License-Identifier: GPL-2.0-or-later
//
// Authors:
//   Michael Kowalski
//
// Copyright (c) 2026 Authors
//

#ifndef INKSCAPE_TEXT_UTILS_H
#define INKSCAPE_TEXT_UTILS_H

#include <vector>
#include <glibmm/ustring.h>
#include "style-enums.h"

class SPItem;

namespace Inkscape::UI {

// Input:
// rtl - text direction right-to-left
// text_align - text alignment
// Output:
// index 0..3 of the button to highlight, where buttons are left, center, right, justify
//
int get_text_align_button_index(bool rtl, SPCSSTextAlign text_align);

// State of a queried text property across one or more items.
enum class PropState { Unset, Single, Mixed };

// Resolved text property values with per-property mixed-state flags.
// For mixed properties, the value is from the first encountered style.
struct TextProperties {
    // numeric
    struct { double value = 0; PropState state = PropState::Unset; } font_size; // value in px
    struct { double value = 0; PropState state = PropState::Unset; } line_height;
    struct { int unit = 0; } line_height_unit;
    struct { double value = 0; PropState state = PropState::Unset; } letter_spacing;
    struct { double value = 0; PropState state = PropState::Unset; } word_spacing;
    // font identity
    struct { Glib::ustring family; PropState state = PropState::Unset; } font_family;
    struct { Glib::ustring style; PropState state = PropState::Unset; } font_style;
    // enums
    struct { int value = -1; PropState state = PropState::Unset; } text_align;   // button index 0-3
    struct { int value = 0; PropState state = PropState::Unset; } writing_mode;
    struct { int value = 0; PropState state = PropState::Unset; } direction;
    struct { int value = 0; PropState state = PropState::Unset; } text_orientation;
    // booleans / toggles
    struct { bool value = false; PropState state = PropState::Unset; } superscript;
    struct { bool value = false; PropState state = PropState::Unset; } subscript;
    struct { bool value = false; PropState state = PropState::Unset; } underline;
    struct { bool value = false; PropState state = PropState::Unset; } overline;
    struct { bool value = false; PropState state = PropState::Unset; } strikethrough;
};

// Query text properties from a list of items (tspans, flowparas, or text elements).
// First item's values are used as baseline; subsequent items flag Mixed if different.
TextProperties query_text_properties(const std::vector<SPItem*>& items);

}

#endif //INKSCAPE_TEXT_UTILS_H
