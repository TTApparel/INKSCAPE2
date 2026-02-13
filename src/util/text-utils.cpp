// SPDX-License-Identifier: GPL-2.0-or-later
//
// Authors:
//   Michael Kowalski
//
// Copyright (c) 2026 Authors
//

#include "text-utils.h"

#include <cstring>
#include "desktop-style.h"
#include "object/sp-flowdiv.h"
#include "object/sp-flowtext.h"
#include "object/sp-item.h"
#include "object/sp-text.h"
#include "object/sp-textpath.h"
#include "object/sp-tref.h"
#include "object/sp-tspan.h"
#include "style.h"
#include "text-editing.h"
#include "ui/tools/text-tool.h"
#include "xml/repr.h"

namespace Inkscape::UI {

namespace {

bool is_textual_item(SPObject const* obj) {
    return is<SPText>(obj)
        || is<SPFlowtext>(obj)
        || is<SPTSpan>(obj)
        || is<SPTRef>(obj)
        || is<SPTextPath>(obj)
        || is<SPFlowdiv>(obj)
        || is<SPFlowpara>(obj)
        || is<SPFlowtspan>(obj);
}

} // anonymous namespace

TextProperties query_text_properties(const std::vector<SPItem*>& items) {
    TextProperties props;
    bool first = true;

    for (auto item : items) {
        if (!item || !is_textual_item(item)) continue;

        auto* style = item->style;
        if (!style) continue;

        // --- font family ---
        auto family = style->font_family.value() ? Glib::ustring(style->font_family.value()) : Glib::ustring();
        if (first) {
            props.font_family.family = family;
            props.font_family.state = PropState::Single;
        } else if (props.font_family.state != PropState::Mixed && props.font_family.family != family) {
            props.font_family.state = PropState::Mixed;
        }

        // --- font style (specification string for face lookup) ---
        auto spec = style->font_specification.set && style->font_specification.value()
            ? Glib::ustring(style->font_specification.value()) : Glib::ustring();
        if (first) {
            props.font_style.style = spec;
            props.font_style.state = PropState::Single;
        } else if (props.font_style.state != PropState::Mixed && props.font_style.style != spec) {
            props.font_style.state = PropState::Mixed;
        }

        // --- font size ---
        double sz = style->font_size.computed;
        if (first) {
            props.font_size.value = sz;
            props.font_size.state = PropState::Single;
        } else if (props.font_size.state != PropState::Mixed && props.font_size.value != sz) {
            props.font_size.state = PropState::Mixed;
        }

        // --- line height ---
        double lh = style->line_height.computed;
        int lh_unit = style->line_height.unit;
        if (style->line_height.normal) {
            lh = -1; // sentinel for "normal"
        } else if (style->line_height.unit == SP_CSS_UNIT_NONE ||
                   style->line_height.unit == SP_CSS_UNIT_PERCENT ||
                   style->line_height.unit == SP_CSS_UNIT_EM ||
                   style->line_height.unit == SP_CSS_UNIT_EX) {
            lh = style->line_height.value;
        }
        if (first) {
            props.line_height.value = lh;
            props.line_height.state = style->line_height.set ? PropState::Single : PropState::Unset;
            props.line_height_unit.unit = lh_unit;
        } else if (props.line_height.state != PropState::Mixed && props.line_height.value != lh) {
            props.line_height.state = PropState::Mixed;
        }

        // --- letter spacing ---
        double ls = style->letter_spacing.normal ? 0.0 : style->letter_spacing.computed;
        if (first) {
            props.letter_spacing.value = ls;
            props.letter_spacing.state = PropState::Single;
        } else if (props.letter_spacing.state != PropState::Mixed && props.letter_spacing.value != ls) {
            props.letter_spacing.state = PropState::Mixed;
        }

        // --- word spacing ---
        double ws = style->word_spacing.normal ? 0.0 : style->word_spacing.computed;
        if (first) {
            props.word_spacing.value = ws;
            props.word_spacing.state = PropState::Single;
        } else if (props.word_spacing.state != PropState::Mixed && props.word_spacing.value != ws) {
            props.word_spacing.state = PropState::Mixed;
        }

        // --- text align ---
        bool rtl = (style->direction.computed == SP_CSS_DIRECTION_RTL);
        int align_idx = get_text_align_button_index(rtl, static_cast<SPCSSTextAlign>(style->text_align.computed));
        if (first) {
            props.text_align.value = align_idx;
            props.text_align.state = PropState::Single;
        } else if (props.text_align.state != PropState::Mixed && props.text_align.value != align_idx) {
            props.text_align.state = PropState::Mixed;
        }

        // --- writing mode ---
        int wm = style->writing_mode.computed;
        if (first) {
            props.writing_mode.value = wm;
            props.writing_mode.state = PropState::Single;
        } else if (props.writing_mode.state != PropState::Mixed && props.writing_mode.value != wm) {
            props.writing_mode.state = PropState::Mixed;
        }

        // --- direction ---
        int dir = style->direction.computed;
        if (first) {
            props.direction.value = dir;
            props.direction.state = PropState::Single;
        } else if (props.direction.state != PropState::Mixed && props.direction.value != dir) {
            props.direction.state = PropState::Mixed;
        }

        // --- text orientation ---
        int orient = style->text_orientation.computed;
        if (first) {
            props.text_orientation.value = orient;
            props.text_orientation.state = PropState::Single;
        } else if (props.text_orientation.state != PropState::Mixed && props.text_orientation.value != orient) {
            props.text_orientation.state = PropState::Mixed;
        }

        // --- baseline shift (superscript / subscript) ---
        bool is_super = style->baseline_shift.set &&
            style->baseline_shift.type == SP_BASELINE_SHIFT_LITERAL &&
            style->baseline_shift.literal == SP_CSS_BASELINE_SHIFT_SUPER;
        bool is_sub = style->baseline_shift.set &&
            style->baseline_shift.type == SP_BASELINE_SHIFT_LITERAL &&
            style->baseline_shift.literal == SP_CSS_BASELINE_SHIFT_SUB;
        if (first) {
            props.superscript.value = is_super;
            props.superscript.state = PropState::Single;
            props.subscript.value = is_sub;
            props.subscript.state = PropState::Single;
        } else {
            if (props.superscript.state != PropState::Mixed && props.superscript.value != is_super) {
                props.superscript.state = PropState::Mixed;
            }
            if (props.subscript.state != PropState::Mixed && props.subscript.value != is_sub) {
                props.subscript.state = PropState::Mixed;
            }
        }

        // --- text decorations ---
        bool ul = style->text_decoration_line.underline;
        bool ol = style->text_decoration_line.overline;
        bool st = style->text_decoration_line.line_through;
        if (first) {
            props.underline.value = ul;
            props.underline.state = PropState::Single;
            props.overline.value = ol;
            props.overline.state = PropState::Single;
            props.strikethrough.value = st;
            props.strikethrough.state = PropState::Single;
        } else {
            if (props.underline.state != PropState::Mixed && props.underline.value != ul) {
                props.underline.state = PropState::Mixed;
            }
            if (props.overline.state != PropState::Mixed && props.overline.value != ol) {
                props.overline.state = PropState::Mixed;
            }
            if (props.strikethrough.state != PropState::Mixed && props.strikethrough.value != st) {
                props.strikethrough.state = PropState::Mixed;
            }
        }

        first = false;
    }

    return props;
}

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

void apply_text_css(SPItem* text_item, Tools::TextTool* tool, SPCSSAttr* css) {
    if (!text_item || !css) return;

    // If text tool has a subselection, apply to that range directly
    if (tool && tool->textItem() == text_item && tool->text_sel_start != tool->text_sel_end) {
        sp_te_apply_style(text_item, tool->text_sel_start, tool->text_sel_end, css);
        if (auto sptext = cast<SPText>(text_item)) {
            sptext->rebuildLayout();
            sptext->updateRepr();
        }
        return;
    }

    // No subselection — apply CSS recursively to the text item
    sp_desktop_apply_css_recursive(text_item, css, true);
    text_item->updateRepr();
}

} // namespace
