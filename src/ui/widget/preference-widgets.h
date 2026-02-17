// SPDX-License-Identifier: GPL-2.0-or-later
//
// Created by Michael Kowalski on 10/24/25.
//

#ifndef INKSCAPE_PREFERENCEWIDGETS_H
#define INKSCAPE_PREFERENCEWIDGETS_H

#include <glibmm/property.h>
#include <gtkmm/checkbutton.h>

#include "generic/spin-button.h"
#include "preferences.h"
#include "ui/operation-blocker.h"

namespace Inkscape::UI::Widget {

class UnitMenu;

class PreferenceCheckButton : public Gtk::CheckButton {
public:
    PreferenceCheckButton(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder)
        : Glib::ObjectBase("PrefCheckButton"), Gtk::CheckButton(cobject),
        prop_path(*this, "pref-path"),
        prop_enum(*this, "pref-enum")
    {
        construct();
    }
    PreferenceCheckButton() : Glib::ObjectBase("PrefCheckButton"),
        prop_path(*this, "pref-path"),
        prop_enum(*this, "pref-enum")
    {
        construct();
    }

    Glib::PropertyProxy<Glib::ustring> property_pref_path() {
        return prop_path.get_proxy();
    }

    static Glib::ObjectBase* wrap_new(GObject* o) {
        Glib::RefPtr<Gtk::Builder> builder;
        auto obj = new PreferenceCheckButton(GTK_CHECK_BUTTON(o), builder);
        return Gtk::manage(obj);
    }

    // Register a "new" type in Glib and bind it to the C++ wrapper function
    static void register_type() {
        if (gtype) return;

        PreferenceCheckButton dummy;
        gtype = G_OBJECT_TYPE(dummy.gobj());

        Glib::wrap_register(gtype, PreferenceCheckButton::wrap_new);
    }
private:
    static GType gtype;
    void construct();
    Glib::Property<Glib::ustring> prop_path;
    Glib::Property<int> prop_enum;
    OperationBlocker _updating;
};

class PreferenceSpinButton : public InkSpinButton {
public:
    PreferenceSpinButton(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder)
        : Glib::ObjectBase("PrefSpinButton"), InkSpinButton(cobject),
        prop_path(*this, "pref-path"),
        prop_unit_menu(*this, "unit-menu"),
        prop_value(*this, "pref-value")
    {
        construct();
    }
    PreferenceSpinButton() : Glib::ObjectBase("PrefSpinButton"),
        prop_path(*this, "pref-path"),
        prop_unit_menu(*this, "unit-menu"),
        prop_value(*this, "pref-value")
    {
        construct();
    }

    Glib::PropertyProxy<Glib::ustring> property_pref_path() {
        return prop_path.get_proxy();
    }

    Glib::PropertyProxy<Glib::ustring> property_unit_menu() {
        return prop_unit_menu.get_proxy();
    }

    void bind_unit_menu(UnitMenu& menu);

    static Glib::ObjectBase* wrap_new(GObject* o) {
        Glib::RefPtr<Gtk::Builder> builder;
        auto obj = new PreferenceSpinButton(GTK_WIDGET(o), builder);
        return Gtk::manage(obj);
    }

    // Register a "new" type in Glib and bind it to the C++ wrapper function
    static void register_type() {
        if (gtype) return;

        PreferenceSpinButton dummy;
        gtype = G_OBJECT_TYPE(dummy.gobj());

        Glib::wrap_register(gtype, PreferenceSpinButton::wrap_new);
    }
private:
    static GType gtype;
    void construct();
    void write_unit_pref();
    void load_unit_pref();
    Glib::Property<Glib::ustring> prop_path;
    Glib::Property<Glib::ustring> prop_unit_menu;
    Glib::Property<double> prop_value;
    UnitMenu* _unit_menu = nullptr;
    Glib::ustring _last_unit;
    OperationBlocker _updating;
};


}

#endif //INKSCAPE_PREFERENCEWIDGETS_H
