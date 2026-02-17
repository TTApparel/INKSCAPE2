// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef INKSCAPE_UI_WIDGET_PREFERENCE_UNIT_MENU_H
#define INKSCAPE_UI_WIDGET_PREFERENCE_UNIT_MENU_H

#include <gtkmm/box.h>
#include <gtkmm/builder.h>

#include "ui/widget/gtk-registry.h"
#include "ui/widget/unit-menu.h"

namespace Inkscape::UI::Widget {

class PreferenceUnitMenu : public BuildableWidget<PreferenceUnitMenu, Gtk::Box>
{
public:
    PreferenceUnitMenu();
    explicit PreferenceUnitMenu(GtkWidget *cobject, const Glib::RefPtr<Gtk::Builder>& builder = {});

    UnitMenu& get_unit_menu() { return *_menu; }
    UnitMenu const& get_unit_menu() const { return *_menu; }

    Glib::SignalProxyProperty signal_changed() { return _menu->signal_changed(); }

private:
    void construct();

    UnitMenu* _menu = nullptr;
};

} // namespace Inkscape::UI::Widget

#endif // INKSCAPE_UI_WIDGET_PREFERENCE_UNIT_MENU_H
