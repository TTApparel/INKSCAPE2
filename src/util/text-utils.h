// SPDX-License-Identifier: GPL-2.0-or-later
//
// Authors:
//   Michael Kowalski
//
// Copyright (c) 2026 Authors
//

#ifndef INKSCAPE_TEXT_UTILS_H
#define INKSCAPE_TEXT_UTILS_H

#include "style-enums.h"

namespace Inkscape::UI {

// Input:
// rtl - text direction right-to-left
// text_align - text alignment
// Output:
// index 0..3 of the button to highlight, where buttons are left, center, right, justify
//
int get_text_align_button_index(bool rtl, SPCSSTextAlign text_align);

}

#endif //INKSCAPE_TEXT_UTILS_H
