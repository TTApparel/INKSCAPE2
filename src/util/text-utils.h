// SPDX-License-Identifier: GPL-2.0-or-later
//
// Authors:
//   Michael Kowalski
//
// Copyright (c) 2026 Authors
//

#ifndef INKSCAPE_TEXT_UTILS_H
#define INKSCAPE_TEXT_UTILS_H

#include <optional>
#include <vector>
#include <glibmm/ustring.h>
#include <pangomm.h>
#include "colors/color.h"
#include "style-enums.h"

class SPCSSAttr;
class SPDesktop;
class SPItem;
class SPText;

namespace Inkscape {

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
    // decoration extras
    struct { int value = 0; PropState state = PropState::Unset; } decoration_style; // 0=solid,1=double,2=dotted,3=dashed,4=wavy
    struct { std::optional<Colors::Color> color; PropState state = PropState::Unset; } decoration_color;
};

// Query text properties from a list of items (tspans, flowparas, or text elements).
// First item's values are used as baseline; subsequent items flag Mixed if different.
TextProperties query_text_properties(const std::vector<SPItem*>& items);

namespace UI::Tools { class TextTool; }

// Resolve start/end text-align to left/right based on text direction.
SPCSSTextAlign text_align_to_side(SPCSSTextAlign align, SPCSSDirection direction);

// Apply text alignment to an SPText item: sets text-anchor + text-align CSS,
// adjusts the text anchor position to preserve the visual bounding box,
// and triggers a display update. Does NOT call DocumentUndo.
// Returns true if the text position was moved.
bool apply_text_alignment(SPText* text, int align_mode);

// Fill CSS attributes from a Pango font description: sets font-family, font-style,
// font-weight, font-stretch, and font-variant. Mirrors FontLister::fill_css but takes
// a FontDescription directly instead of going through FontLister.
void fill_css_from_font_description(SPCSSAttr* css, const Glib::ustring& family,
                                     const Pango::FontDescription& desc);

// Apply character rotation at the text tool's cursor/selection position.
// Computes delta from current rotation and calls sp_te_adjust_rotation.
// Returns true if rotation was applied. Does NOT call DocumentUndo.
bool apply_text_char_rotation(UI::Tools::TextTool* tool, SPDesktop* desktop, double new_degrees);

// Query character rotation at the text tool's cursor position.
// Returns the rotation in degrees (-180..180), or nullopt if unavailable.
std::optional<double> query_text_char_rotation(UI::Tools::TextTool* tool);

// Apply CSS to text: if text tool has a subselection, apply to that range via sp_te_apply_style;
// otherwise apply recursively to the text item. Does NOT call DocumentUndo — caller is
// responsible for maybeDone with a per-property undo key.
void apply_text_css(SPItem* text_item, UI::Tools::TextTool* tool, SPCSSAttr* css);

}

#endif //INKSCAPE_TEXT_UTILS_H
