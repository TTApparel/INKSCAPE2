// SPDX-License-Identifier: GPL-2.0-or-later

#include "preference-unit-menu.h"

namespace Inkscape::UI::Widget {

void PreferenceUnitMenu::construct()
{
    set_name("PreferenceUnitMenu");

    set_orientation(Gtk::Orientation::HORIZONTAL);

    _menu = Gtk::make_managed<UnitMenu>();
    append(*_menu);
}

PreferenceUnitMenu::PreferenceUnitMenu()
    : Glib::ObjectBase("PreferenceUnitMenu")
{
    construct();
}

PreferenceUnitMenu::PreferenceUnitMenu(GtkWidget* cobject, const Glib::RefPtr<Gtk::Builder>& builder)
    : Glib::ObjectBase("PreferenceUnitMenu")
    , BuildableWidget(reinterpret_cast<BaseObjectType*>(cobject), builder)
{
    construct();
}

} // namespace Inkscape::UI::Widget
