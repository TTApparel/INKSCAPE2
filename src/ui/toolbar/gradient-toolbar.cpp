// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file Gradient toolbar
 */
/*
 * Authors:
 *   bulia byak <bulia@dr.com>
 *   Johan Engelen <j.b.c.engelen@ewi.utwente.nl>
 *   Abhishek Sharma
 *   Vaibhav Malik <vaibhavmalik2018@gmail.com>
 *
 * Copyright (C) 2007 Johan Engelen
 * Copyright (C) 2005 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "gradient-toolbar.h"

#include <glibmm/i18n.h>
#include <gtkmm/button.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/togglebutton.h>
#include <map>

#include "desktop.h"
#include "document-undo.h"
#include "document.h"
#include "gradient-chemistry.h"
#include "gradient-drag.h"
#include "object/sp-defs.h"
#include "object/sp-linear-gradient.h"
#include "object/sp-radial-gradient.h"
#include "object/sp-stop.h"
#include "selection.h"
#include "style.h"
#include "ui/builder-utils.h"
#include "ui/icon-names.h"
#include "ui/tools/gradient-tool.h"
#include "ui/util.h"
#include "ui/widget/combo-tool-item.h"
#include "ui/widget/gradient-image.h"
#include "ui/widget/gradient-vector-selector.h"
#include "ui/widget/spinbutton.h"

using Inkscape::DocumentUndo;
using Inkscape::UI::Tools::ToolBase;

namespace Inkscape::UI::Toolbar {
namespace {

void gr_apply_gradient_to_item(SPItem *item, SPGradient *gr, SPGradientType initialType, PaintTarget initialMode, PaintTarget mode)
{
    auto style = item->style;
    bool is_fill = mode == Inkscape::FOR_FILL;
    if (style
        && (is_fill ? style->fill.isPaintserver() : style->stroke.isPaintserver())
        && (is_fill ? is<SPGradient>(style->getFillPaintServer()) : is<SPGradient>(style->getStrokePaintServer())))
    {
        auto server = is_fill ? style->getFillPaintServer() : style->getStrokePaintServer();
        if (is<SPLinearGradient>(server)) {
            sp_item_set_gradient(item, gr, SP_GRADIENT_TYPE_LINEAR, mode);
        } else if ( is<SPRadialGradient>(server) ) {
            sp_item_set_gradient(item, gr, SP_GRADIENT_TYPE_RADIAL, mode);
        }
    } else if (initialMode == mode) {
        sp_item_set_gradient(item, gr, initialType, mode);
    }
}

/**
 * Applies gradient vector gr to the gradients attached to the selected dragger of drag, or if none,
 * to all objects in selection. If there was no previous gradient on an item, uses gradient type and
 * fill/stroke setting from preferences to create new default (linear: left/right; radial: centered)
 * gradient.
*/
void gr_apply_gradient(Selection *selection, GrDrag *drag, SPGradient *gr)
{
    auto prefs = Preferences::get();
    auto initialType = static_cast<SPGradientType>(prefs->getInt("/tools/gradient/newgradient", SP_GRADIENT_TYPE_LINEAR));
    auto initialMode = (prefs->getInt("/tools/gradient/newfillorstroke", 1) != 0) ? Inkscape::FOR_FILL : Inkscape::FOR_STROKE;

    // GRADIENTFIXME: make this work for multiple selected draggers.

    // First try selected dragger
    if (drag && !drag->selected.empty()) {
        auto dragger = *drag->selected.begin();
        for(auto draggable : dragger->draggables) { //for all draggables of dragger
            gr_apply_gradient_to_item(draggable->item, gr, initialType, initialMode, draggable->fill_or_stroke);
        }
        return;
    }

    // If no drag or no dragger selected, act on selection
    for (auto item : selection->items()) {
        gr_apply_gradient_to_item(item, gr, initialType, initialMode, initialMode);
    }
}

int gr_vector_list(Glib::RefPtr<Gtk::ListStore> store, SPDesktop *desktop,
                   bool selection_empty, SPGradient *gr_selected, bool gr_multi)
{
    int selected = -1;

    // Get list of gradients in document.
    SPDocument *document = desktop->getDocument();
    std::vector<SPObject *> gradients = document->getResourceList( "gradient" );
    std::map<Glib::ustring, SPGradient *> labels_gradients; // ordered map, so we sort by label.
    for (auto gradient : gradients) {
        auto grad = cast<SPGradient>(gradient);
        if ( grad->hasStops() && !grad->isSolid() ) {
            labels_gradients.emplace(gr_prepare_label(gradient), grad);
        }
    }

    store->clear();
    Inkscape::UI::Widget::ComboToolItemColumns columns;
    Gtk::TreeModel::Row row;

    if (labels_gradients.empty()) {
        // The document has no gradients
        row = *(store->append());
        row[columns.col_label    ] = _("No gradient");
        row[columns.col_tooltip  ] = "";
        row[columns.col_icon     ] = "NotUsed";
        row[columns.col_data     ] = nullptr;
        row[columns.col_sensitive] = true;
        return selected;
    }

    if (selection_empty) {
        // Document has gradients, but nothing is currently selected.
        row = *(store->append());
        row[columns.col_label    ] = _("Nothing selected");
        row[columns.col_tooltip  ] = "";
        row[columns.col_icon     ] = "NotUsed";
        row[columns.col_data     ] = nullptr;
        row[columns.col_sensitive] = true;
        return selected;
    }

    // Document has gradients and a selection.

    if (gr_selected == nullptr) {
        row = *(store->append());
        row[columns.col_label    ] = _("No gradient");
        row[columns.col_tooltip  ] = "";
        row[columns.col_icon     ] = "NotUsed";
        row[columns.col_data     ] = nullptr;
        row[columns.col_sensitive] = true;
    }

    if (gr_multi) {
        row = *(store->append());
        row[columns.col_label    ] = _("Multiple gradients");
        row[columns.col_tooltip  ] = "";
        row[columns.col_icon     ] = "NotUsed";
        row[columns.col_data     ] = nullptr;
        row[columns.col_sensitive] = true;
    }

    int idx = 0;
    for (auto const &[label, gradient] : labels_gradients) {
        Glib::RefPtr<Gdk::Pixbuf> pixbuf = sp_gradient_to_pixbuf_ref(gradient, 64, 16);

        row = *(store->append());
        row[columns.col_label    ] = label;
        row[columns.col_tooltip  ] = "";
        row[columns.col_icon     ] = "NotUsed";
        row[columns.col_pixbuf   ] = pixbuf;
        row[columns.col_data     ] = gradient;
        row[columns.col_sensitive] = true;

        if (gradient == gr_selected) {
            selected = idx;
        }
        idx ++;
    }

    if (gr_multi) {
        selected = 0; // This will show "Multiple Gradients"
    }

    return selected;
}

/*
 * Get the list of gradients of the selected desktop item
 * These are the gradients containing the repeat settings, not the underlying "getVector" href linked gradient.
 */
void gr_get_dt_selected_gradient(Inkscape::Selection *selection, std::vector<SPGradient *> &gr_selected)
{
    SPGradient *gradient = nullptr;

    auto itemlist= selection->items();
    for(auto i=itemlist.begin();i!=itemlist.end();++i){
        SPItem *item = *i;// get the items gradient, not the getVector() version
         SPStyle *style = item->style;
         SPPaintServer *server = nullptr;

         if (style && (style->fill.isPaintserver())) {
             server = item->style->getFillPaintServer();
         }
         if (style && (style->stroke.isPaintserver())) {
             server = item->style->getStrokePaintServer();
         }

         if ( is<SPGradient>(server) ) {
             gradient = cast<SPGradient>(server);
         }
        if (gradient && gradient->isSolid()) {
            gradient = nullptr;
        }

        if (gradient) {
            gr_selected.push_back(gradient);
        }
    }
}

/*
 * Get the current selection and dragger status from the desktop
 */
void gr_read_selection( Inkscape::Selection *selection,
                        GrDrag *drag,
                        SPGradient *&gr_selected,
                        bool &gr_multi,
                        SPGradientSpread &spr_selected,
                        bool &spr_multi )
{
    if (drag && !drag->selected.empty()) {
        // GRADIENTFIXME: make this work for more than one selected dragger?
        GrDragger *dragger = *(drag->selected.begin());
        for(auto draggable : dragger->draggables) { //for all draggables of dragger
            SPGradient *gradient = sp_item_gradient_get_vector(draggable->item, draggable->fill_or_stroke);
            SPGradientSpread spread = sp_item_gradient_get_spread(draggable->item, draggable->fill_or_stroke);

            if (gradient && gradient->isSolid()) {
                gradient = nullptr;
            }

            if (gradient && (gradient != gr_selected)) {
                if (gr_selected) {
                    gr_multi = true;
                } else {
                    gr_selected = gradient;
                }
            }
            if (spread != spr_selected) {
                if (spr_selected != SP_GRADIENT_SPREAD_UNDEFINED) {
                    spr_multi = true;
                } else {
                    spr_selected = spread;
                }
            }
         }
        return;
    }

   // If no selected dragger, read desktop selection
    auto itemlist= selection->items();
    for(auto i=itemlist.begin();i!=itemlist.end();++i){
        SPItem *item = *i;
        SPStyle *style = item->style;

        if (style && (style->fill.isPaintserver())) {
            SPPaintServer *server = item->style->getFillPaintServer();
            if ( is<SPGradient>(server) ) {
                auto gradient = cast<SPGradient>(server)->getVector();
                SPGradientSpread spread = cast<SPGradient>(server)->fetchSpread();

                if (gradient && gradient->isSolid()) {
                    gradient = nullptr;
                }

                if (gradient && (gradient != gr_selected)) {
                    if (gr_selected) {
                        gr_multi = true;
                    } else {
                        gr_selected = gradient;
                    }
                }
                if (spread != spr_selected) {
                    if (spr_selected != SP_GRADIENT_SPREAD_UNDEFINED) {
                        spr_multi = true;
                    } else {
                        spr_selected = spread;
                    }
                }
            }
        }
        if (style && (style->stroke.isPaintserver())) {
            SPPaintServer *server = item->style->getStrokePaintServer();
            if ( is<SPGradient>(server) ) {
                auto gradient = cast<SPGradient>(server)->getVector();
                SPGradientSpread spread = cast<SPGradient>(server)->fetchSpread();

                if (gradient && gradient->isSolid()) {
                    gradient = nullptr;
                }

                if (gradient && (gradient != gr_selected)) {
                    if (gr_selected) {
                        gr_multi = true;
                    } else {
                        gr_selected = gradient;
                    }
                }
                if (spread != spr_selected) {
                    if (spr_selected != SP_GRADIENT_SPREAD_UNDEFINED) {
                        spr_multi = true;
                    } else {
                        spr_selected = spread;
                    }
                }
            }
        }
    }
}

} // namespace

GradientToolbar::GradientToolbar()
    : GradientToolbar{create_builder("toolbar-gradient.ui")}
{}

GradientToolbar::GradientToolbar(Glib::RefPtr<Gtk::Builder> const &builder)
    : Toolbar{get_widget<Gtk::Box>(builder, "gradient-toolbar")}
    , _linked_btn{get_widget<Gtk::ToggleButton>(builder, "_linked_btn")}
    , _stops_reverse_btn{get_widget<Gtk::Button>(builder, "_stops_reverse_btn")}
    , _offset_item{get_derived_widget<UI::Widget::SpinButton>(builder, "_offset_item")}
    , _stops_add_btn{get_widget<Gtk::Button>(builder, "_stops_add_btn")}
    , _stops_delete_btn{get_widget<Gtk::Button>(builder, "_stops_delete_btn")}
{
    auto prefs = Preferences::get();

    // Setup the spin buttons.
    setup_derived_spin_button(_offset_item, "stopoffset", 0);

    // Values auto-calculated.
    _offset_item.set_custom_numeric_menu_data({});

    // Configure mode buttons
    int btn_index = 0;
    for_each_child(get_widget<Gtk::Box>(builder, "new_type_buttons_box"), [&](Gtk::Widget &item){
        auto &btn = dynamic_cast<Gtk::ToggleButton &>(item);
        _new_type_buttons.push_back(&btn);
        btn.signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &GradientToolbar::new_type_changed), btn_index++));
        return ForEachResult::_continue;
    });

    int mode = prefs->getInt("/tools/gradient/newgradient", SP_GRADIENT_TYPE_LINEAR);
    _new_type_buttons[mode == SP_GRADIENT_TYPE_LINEAR ? 0 : 1]->set_active(); // linear == 1, radial == 2

    btn_index = 0;
    for_each_child(get_widget<Gtk::Box>(builder, "new_fillstroke_buttons_box"), [&](Gtk::Widget &item){
        auto &btn = dynamic_cast<Gtk::ToggleButton &>(item);
        _new_fillstroke_buttons.push_back(&btn);
        btn.signal_clicked().connect(
            sigc::bind(sigc::mem_fun(*this, &GradientToolbar::new_fillstroke_changed), btn_index++));
        return ForEachResult::_continue;
    });

    auto fsmode = prefs->getInt("/tools/gradient/newfillorstroke", 1) != 0 ? Inkscape::FOR_FILL : Inkscape::FOR_STROKE;
    _new_fillstroke_buttons[fsmode == Inkscape::FOR_FILL ? 0 : 1]->set_active();

    /* Gradient Select list */
    {
        UI::Widget::ComboToolItemColumns columns;
        auto store = Gtk::ListStore::create(columns);
        Gtk::TreeModel::Row row;
        row = *(store->append());
        row[columns.col_label    ] = _("No gradient");
        row[columns.col_tooltip  ] = "";
        row[columns.col_icon     ] = "NotUsed";
        row[columns.col_sensitive] = true;

        _select_cb = UI::Widget::ComboToolItem::create(_("Select"),         // Label
                                                       "",                  // Tooltip
                                                       "Not Used",          // Icon
                                                       store );             // Tree store
        _select_cb->use_icon( false );
        _select_cb->use_pixbuf( true );
        _select_cb->use_group_label( true );
        _select_cb->set_active( 0 );
        _select_cb->set_sensitive( false );

        get_widget<Gtk::Box>(builder, "select_box").append(*_select_cb);
        _select_cb->signal_changed().connect(sigc::mem_fun(*this, &GradientToolbar::gradient_changed));
    }

    // Configure the linked button.
    _linked_btn.signal_toggled().connect(sigc::mem_fun(*this, &GradientToolbar::linked_changed));

    bool linkedmode = prefs->getBool("/options/forkgradientvectors/value", true);
    _linked_btn.set_active(!linkedmode);

    // Configure the reverse button.
    _stops_reverse_btn.signal_clicked().connect(sigc::mem_fun(*this, &GradientToolbar::reverse));
    _stops_reverse_btn.set_sensitive(false);

    // Gradient Spread type (how a gradient is drawn outside its nominal area)
    {
        UI::Widget::ComboToolItemColumns columns;
        Glib::RefPtr<Gtk::ListStore> store = Gtk::ListStore::create(columns);

        std::vector<gchar*> spread_dropdown_items_list = {
            const_cast<gchar *>(C_("Gradient repeat type", "None")),
            _("Reflected"),
            _("Direct")
        };

        for (auto item: spread_dropdown_items_list) {
            Gtk::TreeModel::Row row = *(store->append());
            row[columns.col_label    ] = item;
            row[columns.col_sensitive] = true;
        }

        _spread_cb = Gtk::manage(UI::Widget::ComboToolItem::create(_("Repeat"),
                                        // TRANSLATORS: for info, see http://www.w3.org/TR/2000/CR-SVG-20000802/pservers.html#LinearGradientSpreadMethodAttribute
                                        _("Whether to fill with flat color beyond the ends of the gradient vector "
                                          "(spreadMethod=\"pad\"), or repeat the gradient in the same direction "
                                          "(spreadMethod=\"repeat\"), or repeat the gradient in alternating opposite "
                                          "directions (spreadMethod=\"reflect\")"),
                                          "Not Used", store));
        _spread_cb->use_group_label(true);

        _spread_cb->set_active(0);
        _spread_cb->set_sensitive(false);

        _spread_cb->signal_changed().connect(sigc::mem_fun(*this, &GradientToolbar::spread_changed));
        get_widget<Gtk::Box>(builder, "spread_box").append(*_spread_cb);
    }

    // Gradient Stop list
    {
        UI::Widget::ComboToolItemColumns columns;

        auto store = Gtk::ListStore::create(columns);

        Gtk::TreeModel::Row row;

        row = *(store->append());
        row[columns.col_label    ] = _("No stops");
        row[columns.col_tooltip  ] = "";
        row[columns.col_icon     ] = "NotUsed";
        row[columns.col_sensitive] = true;

        _stop_cb =
            UI::Widget::ComboToolItem::create(_("Stops" ),         // Label
                                              "",                  // Tooltip
                                              "Not Used",          // Icon
                                              store );             // Tree store

        _stop_cb->use_icon( false );
        _stop_cb->use_pixbuf( true );
        _stop_cb->use_group_label( true );
        _stop_cb->set_active( 0 );
        _stop_cb->set_sensitive( false );

        get_widget<Gtk::Box>(builder, "stop_box").append(*_stop_cb);
        _stop_cb->signal_changed().connect(sigc::mem_fun(*this, &GradientToolbar::stop_changed));
    }

    // Configure the stops add button.
    _stops_add_btn.signal_clicked().connect(sigc::mem_fun(*this, &GradientToolbar::add_stop));
    _stops_add_btn.set_sensitive(false);

    // Configure the stops add button.
    _stops_delete_btn.signal_clicked().connect(sigc::mem_fun(*this, &GradientToolbar::remove_stop));
    _stops_delete_btn.set_sensitive(false);

    _initMenuBtns();
}

GradientToolbar::~GradientToolbar() = default;

void GradientToolbar::setDesktop(SPDesktop *desktop)
{
    if (_desktop) {
        _connection_changed.disconnect();
        _connection_modified.disconnect();
        _connection_subselection_changed.disconnect();
        _connection_defs_release.disconnect();
        _connection_defs_modified.disconnect();
    }

    Toolbar::setDesktop(desktop);

    if (_desktop) {
        auto sel = desktop->getSelection();
        auto document = desktop->getDocument();

        // connect to selection modified and changed signals
        _connection_changed = sel->connectChanged([this] (auto) { _update(); });
        _connection_modified = sel->connectModified([this] (auto, auto, auto) { _update(); });
        _connection_subselection_changed = desktop->connect_gradient_stop_selected([this] (auto) { _update(); });
        _update();

        // connect to release and modified signals of the defs (i.e. when someone changes gradient)
        _connection_defs_release = document->getDefs()->connectRelease([this] (auto) { _update(); });
        _connection_defs_modified = document->getDefs()->connectModified([this] (auto, auto, auto) { _update(); });
    }
}

void GradientToolbar::setup_derived_spin_button(UI::Widget::SpinButton &btn, Glib::ustring const &name, double default_value)
{
    auto const prefs = Preferences::get();
    auto const path = "/tools/gradient/" + name;
    auto const val = prefs->getDouble(path, default_value);

    auto adj = btn.get_adjustment();
    adj->set_value(val);

    adj->signal_value_changed().connect(sigc::mem_fun(*this, &GradientToolbar::stop_offset_adjustment_changed));

    btn.set_sensitive(false);
    btn.setDefocusTarget(this);
}

void GradientToolbar::new_type_changed(int mode)
{
    Preferences::get()->setInt("/tools/gradient/newgradient",
                               mode == 0 ? SP_GRADIENT_TYPE_LINEAR : SP_GRADIENT_TYPE_RADIAL);
}

void GradientToolbar::new_fillstroke_changed(int mode)
{
    Preferences::get()->setInt("/tools/gradient/newfillorstroke", mode == 0 ? 1 : 0);
}

/*
 * User selected a gradient from the combobox
 */
void GradientToolbar::gradient_changed(int active)
{
    if (_blocker.pending()) {
        return;
    }

    if (active < 0) {
        return;
    }

    auto gr = get_selected_gradient();
    if (!gr) {
        return;
    }

    auto guard = _blocker.block();

    gr = sp_gradient_ensure_vector_normalized(gr);

    auto selection = _desktop->getSelection();
    auto ev = _desktop->getTool();

    gr_apply_gradient(selection, ev ? ev->get_drag() : nullptr, gr);

    DocumentUndo::done(_desktop->getDocument(), _("Assign gradient to object"), INKSCAPE_ICON("color-gradient"));
}

/**
 * \brief Return gradient selected in menu
 */
SPGradient *GradientToolbar::get_selected_gradient()
{
    int active = _select_cb->get_active();

    auto store = _select_cb->get_store();
    auto row   = store->children()[active];
    UI::Widget::ComboToolItemColumns columns;

    void* pointer = row[columns.col_data];
    SPGradient *gr = static_cast<SPGradient *>(pointer);

    return gr;
}

/**
 * \brief User selected a spread method from the combobox
 */
void GradientToolbar::spread_changed(int active)
{
    if (_blocker.pending()) {
        return;
    }

    auto guard = _blocker.block();

    auto selection = _desktop->getSelection();
    std::vector<SPGradient *> gradientList;
    gr_get_dt_selected_gradient(selection, gradientList);

    auto spread = static_cast<SPGradientSpread>(active);

    if (!gradientList.empty()) {
        for (auto item : gradientList) {
            item->setSpread(spread);
            item->updateRepr();
        }
        DocumentUndo::done(_desktop->getDocument(), _("Set gradient repeat"), INKSCAPE_ICON("color-gradient"));
    }
}

/**
 * \brief User selected a stop from the combobox
 */
void GradientToolbar::stop_changed(int active)
{
    if (_blocker.pending()) {
        return;
    }

    auto guard = _blocker.block();

    select_dragger_by_stop(get_selected_gradient(), _desktop->getTool());
}

void GradientToolbar::select_dragger_by_stop(SPGradient *gradient, ToolBase *ev)
{
    if (!_blocker.pending()) {
        std::cerr << "select_dragger_by_stop: should be blocked!" << std::endl;
    }

    if (!ev || !gradient) {
        return;
    }

    auto drag = ev->get_drag();
    if (!drag) {
        return;
    }

    drag->selectByStop(get_selected_stop(), false, true);

    stop_set_offset();
}

/**
 * \brief Get stop selected by menu
 */
SPStop *GradientToolbar::get_selected_stop()
{
    int active = _stop_cb->get_active();

    auto store = _stop_cb->get_store();
    auto row   = store->children()[active];
    UI::Widget::ComboToolItemColumns columns;
    void* pointer = row[columns.col_data];
    return static_cast<SPStop *>(pointer);
}

/**
 *  Change desktop dragger selection to this stop
 *
 *  Set the offset widget value (based on which stop is selected)
 */
void GradientToolbar::stop_set_offset()
{
    if (!_blocker.pending()) {
        std::cerr << "gr_stop_set_offset: should be blocked!" << std::endl;
    }

    auto stop = get_selected_stop();
    if (!stop) {
        // std::cerr << "gr_stop_set_offset: no stop!" << std::endl;
        return;
    }

    SPStop *prev = nullptr;
    prev = stop->getPrevStop();
    auto adj = _offset_item.get_adjustment();
    adj->set_lower(prev != nullptr ? prev->offset : 0);

    SPStop *next = nullptr;
    next = stop->getNextStop();
    adj->set_lower(next != nullptr ? next->offset : 1.0);
    adj->set_value(stop->offset);
    _offset_item.set_sensitive(true);
}

/**
 * \brief User changed the offset
 */
void GradientToolbar::stop_offset_adjustment_changed()
{
    if (_blocker.pending()) {
        return;
    }

    auto guard = _blocker.block();

    auto stop = get_selected_stop();
    if (!stop) {
        return;
    }

    stop->offset = _offset_item.get_adjustment()->get_value();
    _offset_adj_changed = true; // checked to stop changing the selected stop after the update of the offset
    stop->getRepr()->setAttributeCssDouble("offset", stop->offset);

    DocumentUndo::maybeDone(stop->document, "gradient:stop:offset", _("Change gradient stop offset"), INKSCAPE_ICON("color-gradient"));
}

/**
 * \brief Add stop to gradient
 */
void GradientToolbar::add_stop()
{
    if (!_desktop) {
        return;
    }

    auto selection = _desktop->getSelection();
    if (!selection) {
        return;
    }

    if (auto gt = dynamic_cast<Tools::GradientTool*>(_desktop->getTool())) {
        gt->add_stops_between_selected_stops();
    }
}

/**
 * \brief Remove stop from vector
 */
void GradientToolbar::remove_stop()
{
    if (!_desktop) {
        return;
    }

    auto selection = _desktop->getSelection(); // take from desktop, not from args
    if (!selection) {
        return;
    }

    auto ev = _desktop->getTool();
    if (!ev) {
        return;
    }

    auto drag = ev->get_drag();

    if (drag) {
        drag->deleteSelected();
    }
}

/**
 * \brief Reverse vector
 */
void GradientToolbar::reverse()
{
    sp_gradient_reverse_selected_gradients(_desktop);
}

/**
 * \brief Lock or unlock links
 */
void GradientToolbar::linked_changed()
{
    bool active = _linked_btn.get_active();
    if (active) {
        _linked_btn.set_image_from_icon_name(INKSCAPE_ICON("object-locked"));
    } else {
        _linked_btn.set_image_from_icon_name(INKSCAPE_ICON("object-unlocked"));
    }

    Preferences::get()->setBool("/options/forkgradientvectors/value", !active);
}

/**
 * Core function, setup all the widgets whenever something changes on the desktop
 */
void GradientToolbar::_update()
{
    if (_blocker.pending()) {
        return;
    }

    if (!_desktop) {
        return;
    }

    if (_offset_adj_changed) {        // stops change of selection when offset update event is triggered
        _offset_adj_changed = false;
        return;
    }

    auto guard = _blocker.block();

    auto selection = _desktop->getSelection();
    if (selection) {

        ToolBase *ev = _desktop->getTool();
        GrDrag *drag = nullptr;
        if (ev) {
            drag = ev->get_drag();
        }

        SPGradient *gr_selected = nullptr;
        SPGradientSpread spr_selected = SP_GRADIENT_SPREAD_UNDEFINED;
        bool gr_multi = false;
        bool spr_multi = false;

        gr_read_selection(selection, drag, gr_selected, gr_multi, spr_selected, spr_multi);

        // Gradient selection menu
        auto store = _select_cb->get_store();
        int gradient = gr_vector_list (store, _desktop, selection->isEmpty(), gr_selected, gr_multi);

        if (gradient < 0) {
            // No selection or no gradients
            _select_cb->set_active(0);
            _select_cb->set_sensitive(false);
        } else {
            // Single gradient or multiple gradients
            _select_cb->set_active(gradient);
            _select_cb->set_sensitive(true);
        }

        // Spread menu
        _spread_cb->set_sensitive(gr_selected );
        _spread_cb->set_active(gr_selected ? (int)spr_selected : 0);

        _stops_add_btn.set_sensitive((gr_selected && !gr_multi && drag && !drag->selected.empty()));
        _stops_delete_btn.set_sensitive((gr_selected && !gr_multi && drag && !drag->selected.empty()));
        _stops_reverse_btn.set_sensitive((gr_selected != nullptr));

        _stop_cb->set_sensitive(gr_selected && !gr_multi);
        _offset_item.set_sensitive(!gr_multi);

        update_stop_list(gr_selected, nullptr, gr_multi);
        select_stop_by_draggers(gr_selected, ev);
    }
}

/**
 * \brief Construct stop list
 */
int GradientToolbar::update_stop_list(SPGradient *gradient, SPStop *new_stop, bool gr_multi)
{
    if (!_blocker.pending()) {
        std::cerr << "update_stop_list should be blocked!" << std::endl;
    }

    int selected = -1;

    auto store = _stop_cb->get_store();
    if (!store) {
        return selected;
    }

    store->clear();
    UI::Widget::ComboToolItemColumns columns;
    Gtk::TreeModel::Row row;

    if (gr_multi) {
        row = *(store->append());
        row[columns.col_label    ] = _("Multiple gradients");
        row[columns.col_tooltip  ] = "";
        row[columns.col_icon     ] = "NotUsed";
        row[columns.col_data     ] = nullptr;
        row[columns.col_sensitive] = true;
        selected = 0;
        return selected;
    }

    if (!gradient) {
        // No valid gradient
        row = *(store->append());
        row[columns.col_label    ] = _("No gradient");
        row[columns.col_tooltip  ] = "";
        row[columns.col_icon     ] = "NotUsed";
        row[columns.col_data     ] = nullptr;
        row[columns.col_sensitive] = true;
    } else if (!gradient->hasStops()) {
        // Has gradient but it has no stops
        row = *(store->append());
        row[columns.col_label    ] = _("No stops in gradient");
        row[columns.col_tooltip  ] = "";
        row[columns.col_icon     ] = "NotUsed";
        row[columns.col_data     ] = nullptr;
        row[columns.col_sensitive] = true;
    } else {
        // Gradient has stops
        for (auto& ochild: gradient->children) {
            if (is<SPStop>(&ochild)) {

                auto stop = cast<SPStop>(&ochild);
                Glib::RefPtr<Gdk::Pixbuf> pixbuf = sp_gradstop_to_pixbuf_ref(stop, 32, 16);

                Inkscape::XML::Node *repr = reinterpret_cast<SPItem *>(&ochild)->getRepr();
                Glib::ustring label = gr_ellipsize_text(repr->attribute("id"), 25);

                row = *(store->append());
                row[columns.col_label    ] = label;
                row[columns.col_tooltip  ] = "";
                row[columns.col_icon     ] = "NotUsed";
                row[columns.col_pixbuf   ] = pixbuf;
                row[columns.col_data     ] = stop;
                row[columns.col_sensitive] = true;
            }
        }
    }

    if (new_stop) {
        selected = select_stop_in_list (gradient, new_stop);
    }

    return selected;
}

/**
 * \brief Find position of new_stop in menu.
 */
int GradientToolbar::select_stop_in_list(SPGradient *gradient, SPStop *new_stop)
{
    int i = 0;
    for (auto& ochild: gradient->children) {
        if (is<SPStop>(&ochild)) {
            if (&ochild == new_stop) {
                return i;
            }
            i++;
        }
    }
    return -1;
}

/**
 * \brief Set stop in menu to match stops selected by draggers
 */
void GradientToolbar::select_stop_by_draggers(SPGradient *gradient, ToolBase *ev)
{
    if (!_blocker.pending()) {
        std::cerr << "select_stop_by_draggers should be blocked!" << std::endl;
    }

    if (!ev || !gradient)
        return;

    SPGradient *vector = gradient->getVector();
    if (!vector)
        return;

    GrDrag *drag = ev->get_drag();

    if (!drag || drag->selected.empty()) {
        _stop_cb->set_active(0);
        stop_set_offset();
        return;
    }

    gint n = 0;
    SPStop *stop = nullptr;
    int selected = -1;

    // For all selected draggers
    for(auto dragger : drag->selected) {

        // For all draggables of dragger
         for(auto draggable : dragger->draggables) {

            if (draggable->point_type != POINT_RG_FOCUS) {
                n++;
                if (n > 1) break;
            }

            stop = vector->getFirstStop();

            switch (draggable->point_type) {
                case POINT_LG_MID:
                case POINT_RG_MID1:
                case POINT_RG_MID2:
                    stop = sp_get_stop_i(vector, draggable->point_i);
                    break;
                case POINT_LG_END:
                case POINT_RG_R1:
                case POINT_RG_R2:
                    stop = sp_last_stop(vector);
                    break;
                default:
                    break;
            }
        }
        if (n > 1) break;
    }

    if (n > 1) {
        // Multiple stops selected
        _offset_item.set_sensitive(false);

        // Stop list always updated first... reinsert "Multiple stops" as first entry.
        UI::Widget::ComboToolItemColumns columns;
        auto store = _stop_cb->get_store();

        auto row = *(store->prepend());
        row[columns.col_label    ] = _("Multiple stops");
        row[columns.col_tooltip  ] = "";
        row[columns.col_icon     ] = "NotUsed";
        row[columns.col_sensitive] = true;
        selected = 0;

    } else {
        selected = select_stop_in_list(gradient, stop);
    }

    if (selected < 0) {
        _stop_cb->set_active (0);
        _stop_cb->set_sensitive (false);
    } else {
        _stop_cb->set_active (selected);
        _stop_cb->set_sensitive (true);
        stop_set_offset();
    }
}

} // namespace Inkscape::UI::Toolbar

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
