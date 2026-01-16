// SPDX-License-Identifier: GPL-2.0-or-later
//
// Authors:
//   Michael Kowalski
//
// Copyright (c) 2026 Authors
//

#include "text-utils.h"

namespace Inkscape::UI {

int get_text_align_button_index(bool rtl, SPCSSTextAlign text_align) {
    int activeButton = -1; //prefs->getInt("/tools/text/align_mode", 0);
    // bool rtl = (query.direction.computed == SP_CSS_DIRECTION_RTL);

    if ((text_align == SP_CSS_TEXT_ALIGN_START && !rtl) ||
        (text_align == SP_CSS_TEXT_ALIGN_END   &&  rtl) ||
         text_align == SP_CSS_TEXT_ALIGN_LEFT) {
        activeButton = 0;
    } else if (text_align == SP_CSS_TEXT_ALIGN_CENTER) {
        activeButton = 1;
    } else if ((text_align == SP_CSS_TEXT_ALIGN_START &&  rtl) ||
               (text_align == SP_CSS_TEXT_ALIGN_END   && !rtl) ||
                text_align == SP_CSS_TEXT_ALIGN_RIGHT) {
        activeButton = 2;
    } else if (text_align == SP_CSS_TEXT_ALIGN_JUSTIFY) {
        activeButton = 3;
    }
    return activeButton;
}

} // namespace
