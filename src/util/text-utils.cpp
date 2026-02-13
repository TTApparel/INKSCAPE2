// SPDX-License-Identifier: GPL-2.0-or-later
//
// Authors:
//   Michael Kowalski
//
// Copyright (c) 2026 Authors
//

#include "text-utils.h"

#include <cstring>
#include <glibmm/regex.h>
#include "desktop-style.h"
#include "font-discovery.h"
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

namespace Inkscape {

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

        // --- font style (face style string for combo lookup) ---
        // Build a Pango description from the CSS font properties and extract the face style
        Pango::FontDescription desc;
        if (family.size()) desc.set_family(family.raw());
        desc.set_style(static_cast<Pango::Style>(style->font_style.computed));
        desc.set_weight(static_cast<Pango::Weight>(style->font_weight.computed));
        desc.set_stretch(static_cast<Pango::Stretch>(style->font_stretch.computed));
        auto face_style = get_face_style(desc);
        if (first) {
            props.font_style.style = face_style;
            props.font_style.state = PropState::Single;
        } else if (props.font_style.state != PropState::Mixed && props.font_style.style != face_style) {
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
            props.line_height.state = PropState::Single;
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
        // Only report if baseline_shift is explicitly set on this element
        if (style->baseline_shift.set) {
            bool is_super = style->baseline_shift.type == SP_BASELINE_SHIFT_LITERAL &&
                style->baseline_shift.literal == SP_CSS_BASELINE_SHIFT_SUPER;
            bool is_sub = style->baseline_shift.type == SP_BASELINE_SHIFT_LITERAL &&
                style->baseline_shift.literal == SP_CSS_BASELINE_SHIFT_SUB;
            if (props.superscript.state == PropState::Unset) {
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

        // --- decoration style ---
        int ds = 0; // solid by default
        if (style->text_decoration_style.isdouble) ds = 1;
        else if (style->text_decoration_style.dotted) ds = 2;
        else if (style->text_decoration_style.dashed) ds = 3;
        else if (style->text_decoration_style.wavy) ds = 4;
        if (first) {
            props.decoration_style.value = ds;
            props.decoration_style.state = PropState::Single;
        } else if (props.decoration_style.state != PropState::Mixed && props.decoration_style.value != ds) {
            props.decoration_style.state = PropState::Mixed;
        }

        // --- decoration color ---
        std::optional<Colors::Color> dc;
        if (style->text_decoration_color.set && !style->text_decoration_color.inherit) {
            dc = style->text_decoration_color.getColor();
        }
        if (first) {
            props.decoration_color.color = dc;
            props.decoration_color.state = PropState::Single;
        } else if (props.decoration_color.state != PropState::Mixed && props.decoration_color.color != dc) {
            props.decoration_color.state = PropState::Mixed;
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

SPCSSTextAlign text_align_to_side(SPCSSTextAlign align, SPCSSDirection direction) {
    if ((align == SP_CSS_TEXT_ALIGN_START && direction == SP_CSS_DIRECTION_LTR) ||
        (align == SP_CSS_TEXT_ALIGN_END   && direction == SP_CSS_DIRECTION_RTL)) {
        return SP_CSS_TEXT_ALIGN_LEFT;
    }
    if ((align == SP_CSS_TEXT_ALIGN_START && direction == SP_CSS_DIRECTION_RTL) ||
        (align == SP_CSS_TEXT_ALIGN_END   && direction == SP_CSS_DIRECTION_LTR)) {
        return SP_CSS_TEXT_ALIGN_RIGHT;
    }
    return align;
}

bool apply_text_alignment(SPText* text, int align_mode) {
    if (!text || align_mode < 0 || align_mode > 3) return false;

    // Determine axis based on writing mode
    Geom::Dim2 axis;
    unsigned writing_mode = text->style->writing_mode.value;
    if (writing_mode == SP_CSS_WRITING_MODE_LR_TB || writing_mode == SP_CSS_WRITING_MODE_RL_TB) {
        axis = Geom::X;
    } else {
        axis = Geom::Y;
    }

    // Get text bounding box for position adjustment
    Geom::OptRect bbox = text->get_frame();
    if (!bbox) {
        bbox = text->geometricBounds();
    }
    if (!bbox) return false;

    double width = bbox->dimensions()[axis];
    double move = 0;
    auto direction = text->style->direction.value;

    // Calculate position adjustment based on old alignment
    auto old_side = text_align_to_side(text->style->text_align.value, direction);
    switch (old_side) {
        case SP_CSS_TEXT_ALIGN_LEFT:
            switch (align_mode) {
                case 1: move =  width / 2; break;
                case 2: move =  width;     break;
                default: break;
            }
            break;
        case SP_CSS_TEXT_ALIGN_CENTER:
            switch (align_mode) {
                case 0: move = -width / 2; break;
                case 2: move =  width / 2; break;
                default: break;
            }
            break;
        case SP_CSS_TEXT_ALIGN_RIGHT:
            switch (align_mode) {
                case 0: move = -width;     break;
                case 1: move = -width / 2; break;
                default: break;
            }
            break;
        default:
            break;
    }

    // Set text-anchor and text-align CSS
    SPCSSAttr* css = sp_repr_css_attr_new();
    if ((align_mode == 0 && direction == SP_CSS_DIRECTION_LTR) ||
        (align_mode == 2 && direction == SP_CSS_DIRECTION_RTL)) {
        sp_repr_css_set_property(css, "text-anchor", "start");
        sp_repr_css_set_property(css, "text-align",  "start");
    }
    if ((align_mode == 0 && direction == SP_CSS_DIRECTION_RTL) ||
        (align_mode == 2 && direction == SP_CSS_DIRECTION_LTR)) {
        sp_repr_css_set_property(css, "text-anchor", "end");
        sp_repr_css_set_property(css, "text-align",  "end");
    }
    if (align_mode == 1) {
        sp_repr_css_set_property(css, "text-anchor", "middle");
        sp_repr_css_set_property(css, "text-align",  "center");
    }
    if (align_mode == 3) {
        sp_repr_css_set_property(css, "text-anchor", "start");
        sp_repr_css_set_property(css, "text-align",  "justify");
    }
    text->changeCSS(css, "style");
    sp_repr_css_attr_unref(css);

    // Adjust text position to preserve visual bounding box
    Geom::Point XY = text->attributes.firstXY();
    if (axis == Geom::X) {
        XY += Geom::Point(move, 0);
    } else {
        XY += Geom::Point(0, move);
    }
    text->attributes.setFirstXY(XY);
    text->updateRepr();
    text->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);

    return std::abs(move) > 0;
}

void fill_css_from_font_description(SPCSSAttr* css, const Glib::ustring& family,
                                     const Pango::FontDescription& desc) {
    if (!css) return;

    // font-family — properly quoted for CSS
    Glib::ustring quoted_family = family;
    css_font_family_quote(quoted_family);
    sp_repr_css_set_property(css, "font-family", quoted_family.c_str());

    // font-weight — full Pango weight mapping (matches FontLister::fill_css)
    auto weight = static_cast<int>(desc.get_weight());
    switch (weight) {
        case PANGO_WEIGHT_THIN:       sp_repr_css_set_property(css, "font-weight", "100"); break;
        case PANGO_WEIGHT_ULTRALIGHT: sp_repr_css_set_property(css, "font-weight", "200"); break;
        case PANGO_WEIGHT_LIGHT:      sp_repr_css_set_property(css, "font-weight", "300"); break;
        case PANGO_WEIGHT_SEMILIGHT:  sp_repr_css_set_property(css, "font-weight", "350"); break;
        case PANGO_WEIGHT_BOOK:       sp_repr_css_set_property(css, "font-weight", "380"); break;
        case PANGO_WEIGHT_NORMAL:     sp_repr_css_set_property(css, "font-weight", "normal"); break;
        case PANGO_WEIGHT_MEDIUM:     sp_repr_css_set_property(css, "font-weight", "500"); break;
        case PANGO_WEIGHT_SEMIBOLD:   sp_repr_css_set_property(css, "font-weight", "600"); break;
        case PANGO_WEIGHT_BOLD:       sp_repr_css_set_property(css, "font-weight", "bold"); break;
        case PANGO_WEIGHT_ULTRABOLD:  sp_repr_css_set_property(css, "font-weight", "800"); break;
        case PANGO_WEIGHT_HEAVY:      sp_repr_css_set_property(css, "font-weight", "900"); break;
        case PANGO_WEIGHT_ULTRAHEAVY: sp_repr_css_set_property(css, "font-weight", "1000"); break;
        default:
            if (weight > 0 && weight < 1000) {
                sp_repr_css_set_property(css, "font-weight", std::to_string(weight).c_str());
            }
            break;
    }

    // font-style
    switch (desc.get_style()) {
        case Pango::Style::NORMAL:  sp_repr_css_set_property(css, "font-style", "normal"); break;
        case Pango::Style::OBLIQUE: sp_repr_css_set_property(css, "font-style", "oblique"); break;
        case Pango::Style::ITALIC:  sp_repr_css_set_property(css, "font-style", "italic"); break;
    }

    // font-stretch
    switch (desc.get_stretch()) {
        case Pango::Stretch::ULTRA_CONDENSED: sp_repr_css_set_property(css, "font-stretch", "ultra-condensed"); break;
        case Pango::Stretch::EXTRA_CONDENSED: sp_repr_css_set_property(css, "font-stretch", "extra-condensed"); break;
        case Pango::Stretch::CONDENSED:       sp_repr_css_set_property(css, "font-stretch", "condensed"); break;
        case Pango::Stretch::SEMI_CONDENSED:  sp_repr_css_set_property(css, "font-stretch", "semi-condensed"); break;
        case Pango::Stretch::NORMAL:          sp_repr_css_set_property(css, "font-stretch", "normal"); break;
        case Pango::Stretch::SEMI_EXPANDED:   sp_repr_css_set_property(css, "font-stretch", "semi-expanded"); break;
        case Pango::Stretch::EXPANDED:        sp_repr_css_set_property(css, "font-stretch", "expanded"); break;
        case Pango::Stretch::EXTRA_EXPANDED:  sp_repr_css_set_property(css, "font-stretch", "extra-expanded"); break;
        case Pango::Stretch::ULTRA_EXPANDED:  sp_repr_css_set_property(css, "font-stretch", "ultra-expanded"); break;
    }

    // font-variant
    switch (desc.get_variant()) {
        case Pango::Variant::NORMAL:     sp_repr_css_set_property(css, "font-variant", "normal"); break;
        case Pango::Variant::SMALL_CAPS: sp_repr_css_set_property(css, "font-variant", "small-caps"); break;
    }

    // font-variation-settings — convert Pango format "axis=value,..." to CSS "'axis' value, ..."
    auto vars = desc.get_variations();
    if (!vars.empty()) {
        std::string css_vars;
        auto tokens = Glib::Regex::split_simple(",", vars);
        auto regex = Glib::Regex::create("(\\w{4})=([-+]?\\d*\\.?\\d+([eE][-+]?\\d+)?)");
        Glib::MatchInfo match_info;
        for (auto const& token : tokens) {
            regex->match(token, match_info);
            if (match_info.matches()) {
                css_vars += "'";
                css_vars += match_info.fetch(1).raw();
                css_vars += "' ";
                css_vars += match_info.fetch(2).raw();
                css_vars += ", ";
            }
        }
        if (css_vars.length() >= 2) {
            css_vars.pop_back();
            css_vars.pop_back();
        }
        sp_repr_css_set_property(css, "font-variation-settings", css_vars.c_str());
    } else {
        sp_repr_css_unset_property(css, "font-variation-settings");
    }
}

bool apply_text_dx(UI::Tools::TextTool* tool, SPDesktop* desktop, double new_dx) {
    if (!tool || !tool->textItem()) return false;

    unsigned char_index = -1;
    auto attributes = text_tag_attributes_at_position(
        tool->textItem(), std::min(tool->text_sel_start, tool->text_sel_end), &char_index);
    if (!attributes) return false;

    double delta = new_dx - attributes->getDx(char_index);
    sp_te_adjust_dx(tool->textItem(), tool->text_sel_start, tool->text_sel_end, desktop, delta);
    return true;
}

bool apply_text_dy(UI::Tools::TextTool* tool, SPDesktop* desktop, double new_dy) {
    if (!tool || !tool->textItem()) return false;

    unsigned char_index = -1;
    auto attributes = text_tag_attributes_at_position(
        tool->textItem(), std::min(tool->text_sel_start, tool->text_sel_end), &char_index);
    if (!attributes) return false;

    double delta = new_dy - attributes->getDy(char_index);
    sp_te_adjust_dy(tool->textItem(), tool->text_sel_start, tool->text_sel_end, desktop, delta);
    return true;
}

std::optional<double> query_text_dx(UI::Tools::TextTool* tool) {
    if (!tool || !tool->textItem()) return std::nullopt;

    unsigned char_index = -1;
    auto attributes = text_tag_attributes_at_position(
        tool->textItem(), std::min(tool->text_sel_start, tool->text_sel_end), &char_index);
    if (!attributes) return std::nullopt;

    return attributes->getDx(char_index);
}

std::optional<double> query_text_dy(UI::Tools::TextTool* tool) {
    if (!tool || !tool->textItem()) return std::nullopt;

    unsigned char_index = -1;
    auto attributes = text_tag_attributes_at_position(
        tool->textItem(), std::min(tool->text_sel_start, tool->text_sel_end), &char_index);
    if (!attributes) return std::nullopt;

    return attributes->getDy(char_index);
}

bool apply_text_char_rotation(UI::Tools::TextTool* tool, SPDesktop* desktop, double new_degrees) {
    if (!tool || !tool->textItem()) return false;

    unsigned char_index = -1;
    auto attributes = text_tag_attributes_at_position(
        tool->textItem(), std::min(tool->text_sel_start, tool->text_sel_end), &char_index);
    if (!attributes) return false;

    double old_degrees = attributes->getRotate(char_index);
    double delta_deg = new_degrees - old_degrees;
    sp_te_adjust_rotation(tool->textItem(), tool->text_sel_start, tool->text_sel_end, desktop, delta_deg);
    return true;
}

std::optional<double> query_text_char_rotation(UI::Tools::TextTool* tool) {
    if (!tool || !tool->textItem()) return std::nullopt;

    unsigned char_index = -1;
    auto attributes = text_tag_attributes_at_position(
        tool->textItem(), std::min(tool->text_sel_start, tool->text_sel_end), &char_index);
    if (!attributes) return std::nullopt;

    double rotation = attributes->getRotate(char_index);
    // SVG value is 0..360 but we use -180..180
    if (rotation > 180.0) {
        rotation -= 360.0;
    }
    return rotation;
}

void apply_text_css(SPItem* text_item, UI::Tools::TextTool* tool, SPCSSAttr* css) {
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
