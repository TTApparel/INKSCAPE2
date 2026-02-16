// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Author:
 *   Felipe Corrêa da Silva Sanches <juca@members.fsf.org>
 *   Tavmjong Bah <tavmjong@free.fr>
 *   Michael Kowalski <michal_kowalski@hotmail.com>
 *
 * Copyright (C) 2018 Felipe Corrêa da Silva Sanches, Tavmong Bah
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <cmath>
#include <glibmm/refptr.h>
#include <glibmm/ustring.h>
#include <gtkmm/adjustment.h>
#include <gtkmm/enums.h>
#include <gtkmm/object.h>
#include <gtkmm/sizegroup.h>
#include <gtkmm/spinbutton.h>
#include <iostream>
#include <iomanip>
#include <map>

#include <gtkmm.h>
#include <glibmm/i18n.h>

#include <libnrtype/font-instance.h>
#include <string>
#include <utility>
#include "libnrtype/font-factory.h"

#include "font-variations.h"

// For updating from selection
#include "svg/css-ostringstream.h"

#include "ui/util.h"

namespace Inkscape::UI::Widget {

std::pair<Glib::ustring, Glib::ustring> get_axis_name(const std::string& tag, const Glib::ustring& abbr) {
    // Transformed axis names;
    // mainly from https://fonts.google.com/knowledge/using_type/introducing_parametric_axes
    // CC BY-SA 4.0
    // Standard axes guide for reference: https://variationsguide.typenetwork.com
    // Other references:
    // - https://fonts.google.com/variablefonts#axis-definitions
    // - https://canary.grida.co/docs/reference/open-type-variable-axes

    static std::map<std::string, std::pair<Glib::ustring, Glib::ustring>> map = {
        // TRANSLATORS: “Grade” (GRAD in CSS) is an axis that can be used to alter stroke thicknesses (or other forms)
        // without affecting the type's overall width, inter-letter spacing, or kerning — unlike altering weight.
        {"GRAD", std::make_pair(C_("Variable font axis", "Grade"),         _("Alter stroke thicknesses (or other forms) without affecting the type’s overall width"))},
        // TRANSLATORS: “Parametric Thick Stroke”, XOPQ, is a reference to its logical name, “X Opaque”,
        // which describes how it alters the opaque stroke forms of glyphs typically in the X dimension
        {"XOPQ", std::make_pair(C_("Variable font axis", "X opaque"),      _("Alter the opaque stroke forms of glyphs in the X dimension"))},
        // TRANSLATORS: “Parametric Thin Stroke”, YOPQ, is a reference to its logical name, “Y Opaque”,
        // which describes how it alters the opaque stroke forms of glyphs typically in the Y dimension
        {"YOPQ", std::make_pair(C_("Variable font axis", "Y opaque"),      _("Alter the opaque stroke forms of glyphs in the Y dimension"))},
        // TRANSLATORS: “Parametric Counter Width”, XTRA, is a reference to its logical name, “X-Transparent,”
        // which describes how it alters a font’s transparent spaces (also known as negative shapes)
        // inside and around all glyphs along the X dimension
        {"XTRA", std::make_pair(C_("Variable font axis", "X transparent"), _("Alter the transparent spaces inside and around all glyphs along the X dimension"))},
        {"YTRA", std::make_pair(C_("Variable font axis", "Y transparent"), _("Alter the transparent spaces inside and around all glyphs along the Y dimension"))},
        // TRANSLATORS: Width/height of Chinese glyphs
        {"XTCH", std::make_pair(C_("Variable font axis", "X transparent Chinese"), _("Alter the width of Chinese glyphs"))},
        {"YTCH", std::make_pair(C_("Variable font axis", "Y transparent Chinese"), _("Alter the height of Chinese glyphs"))},
        // TRANSLATORS: “Parametric Lowercase Height”
        {"YTLC", std::make_pair(C_("Variable font axis", "Lowercase height"), _("Vary the height of counters and other spaces between the baseline and x-height"))},
        // TRANSLATORS: “Parametric Uppercase Counter Height”
        {"YTUC", std::make_pair(C_("Variable font axis", "Uppercase height"), _("Vary the height of uppercase letterforms"))},
        // TRANSLATORS: “Parametric Ascender Height”
        {"YTAS", std::make_pair(C_("Variable font axis", "Ascender height"),  _("Vary the height of lowercase ascenders"))},
        // TRANSLATORS: “Parametric Descender Depth”
        {"YTDE", std::make_pair(C_("Variable font axis", "Descender depth"),  _("Vary the depth of lowercase descenders"))},
        // TRANSLATORS: “Parametric Figure Height”
        {"YTFI", std::make_pair(C_("Variable font axis", "Figure height"), _("Vary the height of figures"))},
        // TRANSLATORS: "Serif rise" - found in the wild (https://github.com/googlefonts/amstelvar)
        {"YTSE", std::make_pair(C_("Variable font axis", "Serif rise"),  _("Vary the shape of the serifs"))},
        // TRANSLATORS: Flare - flaring of the stems
        {"FLAR", std::make_pair(C_("Variable font axis", "Flare"),         _("Controls the flaring of the stems"))},
        // TRANSLATORS: Volume - The volume axis works only in combination with the Flare axis. It transforms the serifs
        // and adds a little more edge to details.
        {"VOLM", std::make_pair(C_("Variable font axis", "Volume"),        _("Volume works in combination with flare to transform serifs"))},
        // Softness
        {"SOFT", std::make_pair(C_("Variable font axis", "Softness"),      _("Softness makes letterforms more soft and rounded"))},
        // Casual
        {"CASL", std::make_pair(C_("Variable font axis", "Casual"),        _("Adjust the letterforms from a more serious style to a more casual style"))},
        // Cursive
        {"CRSV", std::make_pair(C_("Variable font axis", "Cursive"),       _("Control the substitution of cursive forms"))},
        // Fill
        {"FILL", std::make_pair(C_("Variable font axis", "Fill"),          _("Fill can turn transparent forms opaque"))},
        // Monospace
        {"MONO", std::make_pair(C_("Variable font axis", "Monospace"),     _("Adjust the glyphs from a proportional width to a fixed width"))},
        // Wonky
        {"WONK", std::make_pair(C_("Variable font axis", "Wonky"),         _("Binary switch used to control substitution of “wonky” forms"))},
        // Element shape
        {"ESHP", std::make_pair(C_("Variable font axis", "Element shape"), _("Selection of the base element glyphs are composed of"))},
        // Element shape
        {"ELSH", std::make_pair(C_("Variable font axis", "Element shape"), _("Controls element shape characteristics"))},
        // Element grid
        {"ELGR", std::make_pair(C_("Variable font axis", "Element grid"),  _("Controls how many elements are used per one grid unit"))},
        // Element grid
        {"EGRD", std::make_pair(C_("Variable font axis", "Element grid"),  _("Controls how many elements are used per one grid unit"))},
        // Proposed axis "height"
        {"HGHT", std::make_pair(C_("Variable font axis", "Height"),        _("Controls the font file’s height parameter"))},
        // Non-standard Y-axis stem thickness
        {"YAXS", std::make_pair(C_("Variable font axis", "Y-Axis"),        _("Controls stem thickness in vertical direction"))},
        // Vertical Element Alignment
        {"YELA", std::make_pair(C_("Variable font axis", "Vertical align"), _("Controls vertical element alignment"))},
        // Corner roundness
        {"ROND", std::make_pair(C_("Variable font axis", "Roundness"),     _("Controls corner roundness"))},
        // Bleed
        {"BLED", std::make_pair(C_("Variable font axis", "Bleed"),         _("Controls ink bleed effect"))},
        // Scanlines
        {"SCAN", std::make_pair(C_("Variable font axis", "Scanlines"),     _("Controls scanline effect"))},
        // Morph
        {"MORF", std::make_pair(C_("Variable font axis", "Morph"),         _("Controls morphing characteristics"))},
        // Extrusion
        {"EDPT", std::make_pair(C_("Variable font axis", "Extrusion depth"), _("Controls depth of extrusion"))},
        // Edge highlight
        {"EHLT", std::make_pair(C_("Variable font axis", "Edge highlight"), _("Controls edge highlighting"))},
        // Hyper expansion
        {"HEXP", std::make_pair(C_("Variable font axis", "Hyper expansion"), _("Controls hyper expansion characteristics"))},
        // Bounce
        {"BNCE", std::make_pair(C_("Variable font axis", "Bounce"),        _("Controls bounce/spring effect"))},
        // Informal
        {"INFM", std::make_pair(C_("Variable font axis", "Informality"),   _("Controls informality characteristics"))},
        // Spacing
        {"SPAC", std::make_pair(C_("Variable font axis", "Spacing"),       _("Controls character spacing"))},
        // Negative space
        {"NEGA", std::make_pair(C_("Variable font axis", "Negative space"), _("Controls negative spacing"))},
        // X-rotation
        {"XROT", std::make_pair(C_("Variable font axis", "X rotation"),    _("Controls character 3D horizontal rotation"))},
        // Y-rotation
        {"YROT", std::make_pair(C_("Variable font axis", "Y rotation"),    _("Controls character 3D vertical rotation"))},
        // Sharpness
        {"SHRP", std::make_pair(C_("Variable font axis", "Sharpness"),     _("Controls sharpness characteristics"))},
        // TRANSLATORS: “Optical Size”
        // Optical sizes in a variable font are different versions of a typeface optimized for use at singular specific sizes,
        // such as 14 pt or 144 pt. Small (or body) optical sizes tend to have less stroke contrast, more open and wider spacing,
        // and a taller x-height than those of their large (or display) counterparts.
        {"opsz", std::make_pair(C_("Variable font axis", "Optical size"),  _("Optimize the typeface for use at specific size"))},
        // TRANSLATORS: Slant controls the font file’s slant parameter for oblique styles.
        {"slnt", std::make_pair(C_("Variable font axis", "Slant"),         _("Controls the font file’s slant parameter for oblique styles"))},
        // Italic
        {"ital", std::make_pair(C_("Variable font axis", "Italic"),        _("Turns on the font’s italic forms"))},
        // TRANSLATORS: Weight controls the font file’s weight parameter.
        {"wght", std::make_pair(C_("Variable font axis", "Weight"),        _("Controls the font file’s weight parameter"))},
        // TRANSLATORS: Width controls the font file’s width parameter.
        {"wdth", std::make_pair(C_("Variable font axis", "Width"),         _("Controls the font file’s width parameter"))},
        //
        {"xtab", std::make_pair(C_("Variable font axis", "Tabular width"), _("Controls the tabular width"))},
        {"udln", std::make_pair(C_("Variable font axis", "Underline"),     _("Controls the weight of an underline"))},
        {"shdw", std::make_pair(C_("Variable font axis", "Shadow"),        _("Controls the depth of a shadow"))},
        {"refl", std::make_pair(C_("Variable font axis", "Reflection"),    _("Controls the Y reflection"))},
        {"otln", std::make_pair(C_("Variable font axis", "Outline"),       _("Controls the weight of a font’s outline"))},
        {"engr", std::make_pair(C_("Variable font axis", "Engrave"),       _("Controls the width of an engraving"))},
        {"embo", std::make_pair(C_("Variable font axis", "Emboss"),        _("Controls the depth of an emboss"))},
        {"rxad", std::make_pair(C_("Variable font axis", "Relative X advance"), _("Controls the relative X advance - horizontal motion of the glyph"))},
        {"ryad", std::make_pair(C_("Variable font axis", "Relative Y advance"), _("Controls the relative Y advance - vertical motion of the glyph"))},
        {"rsec", std::make_pair(C_("Variable font axis", "Relative second"),    _("Controls the relative second value - as in one second of animation time"))},
        {"vrot", std::make_pair(C_("Variable font axis", "Rotation"),      _("Controls the rotation of the glyph in degrees"))},
        {"vuid", std::make_pair(C_("Variable font axis", "Unicode variation"),  _("Controls the glyph’s unicode ID"))},
        {"votf", std::make_pair(C_("Variable font axis", "Feature variation"),  _("Controls the glyph’s feature variation"))},
    };

    auto it = map.find(tag);
    if (it == end(map)) {
        // try lowercase variants
        it = map.find(boost::algorithm::to_lower_copy(tag));
    }
    if (it == end(map)) {
        // try uppercase variants
        it = map.find(boost::algorithm::to_upper_copy(tag));
    }
    if (it != end(map)) {
        return it->second;
    }
    else {
        return std::make_pair(abbr, "");
    }
}

FontVariationAxis::FontVariationAxis(Glib::ustring name_, OTVarAxis const &axis, Glib::ustring label_, Glib::ustring tooltip)
    : Gtk::Box(Gtk::Orientation::HORIZONTAL)
      , name(std::move(name_))
{
    // std::cout << "FontVariationAxis::FontVariationAxis:: "
    //           << " name: " << name
    //           << " min:  " << axis.minimum
    //           << " def:  " << axis.def
    //           << " max:  " << axis.maximum
    //           << " val:  " << axis.set_val << std::endl;

    set_spacing(4);

    label = Gtk::make_managed<Gtk::Label>(label_);
    label->set_tooltip_text(tooltip);
    label->set_xalign(0.0f); // left-align
    append(*label);

    edit = Gtk::make_managed<SpinButton>();
    edit->set_max_width_chars(5);
    edit->set_valign(Gtk::Align::CENTER);
    edit->set_margin_top(2);
    edit->set_margin_bottom(2);
    edit->set_tooltip_text(tooltip);
    append(*edit);

    auto magnitude = static_cast<int>(log10(axis.maximum - axis.minimum));
    precision = 2 - magnitude;
    if (precision < 0) precision = 0;

    auto adj = Gtk::Adjustment::create(axis.set_val, axis.minimum, axis.maximum);
    auto step = pow(10.0, -precision);
    adj->set_step_increment(step);
    adj->set_page_increment(step * 10.0);
    edit->set_adjustment(adj);
    edit->set_digits(precision);

    auto adj_scale = Gtk::Adjustment::create(axis.set_val, axis.minimum, axis.maximum);
    adj_scale->set_step_increment(step);
    adj_scale->set_page_increment(step * 10.0);
    scale = Gtk::make_managed<Gtk::Scale>();
    scale->set_digits (precision);
    scale->set_hexpand(true);
    scale->set_adjustment(adj_scale);
    scale->get_style_context()->add_class("small-slider");
    scale->set_draw_value(false);
    append(*scale);

    // sync slider with spin button
    g_object_bind_property(adj->gobj(), "value", adj_scale->gobj(), "value", GBindingFlags(G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL));

    def = axis.def; // Default value
}

void FontVariationAxis::set_value(double value) {
    if (get_value() != value) {
        scale->get_adjustment()->set_value(value);
    }
}

// ------------------------------------------------------------- //

FontVariations::FontVariations()
    : Gtk::Box(Gtk::Orientation::VERTICAL)
{
    // std::cout << "FontVariations::FontVariations" << std::endl;
    set_name("FontVariations");

    _size_group = Gtk::SizeGroup::create(Gtk::SizeGroup::Mode::HORIZONTAL);
    _size_group_edit = Gtk::SizeGroup::create(Gtk::SizeGroup::Mode::HORIZONTAL);
}

// Update GUI based on query.
void FontVariations::update(Glib::ustring const &font_spec)
{
    auto res = FontFactory::get().FaceFromFontSpecification(font_spec.c_str());
    const auto& axes = res ? res->get_opentype_varaxes() : std::map<Glib::ustring, OTVarAxis>();

    bool rebuild = false;
    if (_open_type_axes.size() != axes.size()) {
        rebuild = true;
    }
    else {
        // compare axes
        bool identical = std::equal(begin(axes), end(axes), begin(_open_type_axes));
        // if identical, then there's nothing to do
        if (identical) return;

        bool same_def = std::equal(begin(axes), end(axes), begin(_open_type_axes), [=](const auto& a, const auto& b){
            return a.first == b.first && a.second.same_definition(b.second);
        });

        // different axes definitions?
        if (!same_def) rebuild = true;
    }

    auto scoped(_update.block());

    if (rebuild) {
        // rebuild UI if variable axes definitions have changed
        build_ui(axes);
    }
    else {
        // update UI in-place, some values are different
        auto it = begin(axes);
        for (auto& axis : _axes) {
            if (it != end(axes) && axis->get_name() == it->first) {
                const auto eps = 0.00001;
                if (abs(axis->get_value() - it->second.set_val) > eps) {
                    axis->set_value(it->second.set_val);
                }
            }
            else {
                g_message("axis definition mismatch '%s'", axis->get_name().c_str());
            }
            ++it;
        }
    }

    _open_type_axes = axes;
}


void FontVariations::build_ui(const std::map<Glib::ustring, OTVarAxis>& ot_axes) {
    // remove existing widgets, if any
    auto children = get_children();
    for (auto child : children) {
        if (auto group = dynamic_cast<FontVariationAxis*>(child)) {
            _size_group->remove_widget(*group->get_label());
            _size_group_edit->remove_widget(*group->get_editbox());
        }
        remove(*child);
    }

    _axes.clear();
    // create new widgets
    for (const auto& a : ot_axes) {
        // std::cout << "Creating axis: " << a.first << std::endl;
        auto label_tooltip = get_axis_name(a.second.tag, a.first);
        auto axis = Gtk::make_managed<FontVariationAxis>(a.first, a.second, label_tooltip.first, label_tooltip.second);
        _axes.push_back(axis);
        append(*axis);
        _size_group->add_widget(*(axis->get_label())); // Keep labels the same width
        _size_group_edit->add_widget(*axis->get_editbox());
        axis->get_editbox()->get_adjustment()->signal_value_changed().connect(
            [this](){ if (!_update.pending()) {_signal_changed.emit();} }
        );
        // apply remembered scale visibility
        axis->get_scale()->set_visible(_scales_visible);
    }
}

#if false
void
FontVariations::fill_css( SPCSSAttr *css ) {

    // Eventually will want to favor using 'font-weight', etc. but at the moment these
    // can't handle "fractional" values. See CSS Fonts Module Level 4.
    sp_repr_css_set_property(css, "font-variation-settings", get_css_string().c_str());
}

Glib::ustring
FontVariations::get_css_string() {

    Glib::ustring css_string;

    for (auto axis: axes) {
        Glib::ustring name = axis->get_name();

        // Translate the "named" axes. (Additional names in 'stat' table, may need to handle them.)
        if (name == "Width")  name = "wdth";       // 'font-stretch'
        if (name == "Weight") name = "wght";       // 'font-weight'
        if (name == "OpticalSize") name = "opsz";  // 'font-optical-sizing' Can trigger glyph substitution.
        if (name == "Slant")  name = "slnt";       // 'font-style'
        if (name == "Italic") name = "ital";       // 'font-style' Toggles from Roman to Italic.

        std::stringstream value;
        value << std::fixed << std::setprecision(axis->get_precision()) << axis->get_value();
        css_string += "'" + name + "' " + value.str() + "', ";
    }

    return css_string;
}
#endif

Glib::ustring
FontVariations::get_pango_string(bool include_defaults) const {

    Glib::ustring pango_string;

    if (!_axes.empty()) {

        pango_string += "@";

        for (const auto& axis: _axes) {
            if (!include_defaults && axis->get_value() == axis->get_def()) continue;
            Glib::ustring name = axis->get_name();

            // Translate the "named" axes. (Additional names in 'stat' table, may need to handle them.)
            if (name == "Width")  name = "wdth";       // 'font-stretch'
            if (name == "Weight") name = "wght";       // 'font-weight'
            if (name == "OpticalSize") name = "opsz";  // 'font-optical-sizing' Can trigger glyph substitution.
            if (name == "Slant")  name = "slnt";       // 'font-style'
            if (name == "Italic") name = "ital";       // 'font-style' Toggles from Roman to Italic.

            CSSOStringStream str;
            str << std::fixed << std::setprecision(axis->get_precision()) << axis->get_value();
            pango_string += name + "=" + str.str() + ",";
        }

        pango_string.erase (pango_string.size() - 1); // Erase last ',' or '@'
    }

    return pango_string;
}

bool FontVariations::variations_present() const {
    return !_axes.empty();
}

Glib::RefPtr<Gtk::SizeGroup> FontVariations::get_size_group(int index) {
    switch (index) {
    case 0: return _size_group;
    case 1: return _size_group_edit;
    default: return Glib::RefPtr<Gtk::SizeGroup>();
    }
}

int FontVariations::measure_height(int axis_count) {
    std::map<Glib::ustring, OTVarAxis> axes;
    for (int i = 0; i < axis_count; ++i) {
        auto name = std::to_string(i);
        OTVarAxis axis;
        axis.tag = name;
        axes[name] = axis;
    }
    build_ui(axes);
    int min=0,nat=0,b1,b2;
    measure(Gtk::Orientation::VERTICAL, 9999, min, nat, b1, b2);
    build_ui({});
    return nat;
}

void FontVariations::set_scales_visible(bool visible) {
    if (_scales_visible == visible) return;

    _scales_visible = visible;
    for (auto axis : _axes) {
        axis->get_scale()->set_visible(visible);
    }
}

}

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8 :
