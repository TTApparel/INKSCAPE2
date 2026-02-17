// SPDX-License-Identifier: GPL-2.0-or-later
//
// Created by Michael Kowalski on 6/28/25.
//
// This is a helper widget used to store string path property in a UI file

#ifndef PROPERTY_WIDGET_H
#define PROPERTY_WIDGET_H

#include <glibmm/property.h>
#include <gtkmm/drawingarea.h>

namespace Gtk {
class Builder;
}

namespace Inkscape::UI::Widget {

class PropertyWidget : public Gtk::DrawingArea {
public:
    PropertyWidget();
    PropertyWidget(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder);
    explicit PropertyWidget(BaseObjectType* cobject);

    ~PropertyWidget() override = default;

    void set_design_time(bool design_time);

    // Construct a C++ object from a parent (=base) C class object
    static Glib::ObjectBase* wrap_new(GObject* o) {
        auto obj = new PropertyWidget(GTK_DRAWING_AREA(o));
        return Gtk::manage(obj);
    }

    // Register a "new" type in Glib and bind it to the C++ wrapper function
    static void register_type() {
        if (gtype) return;

        PropertyWidget dummy;
        gtype = G_OBJECT_TYPE(dummy.gobj());

        Glib::wrap_register(gtype, PropertyWidget::wrap_new);
    }

    Glib::PropertyProxy<Glib::ustring> property_path() { return _path.get_proxy(); }

private:
    void construct();
    void draw_text(const Cairo::RefPtr<Cairo::Context>& ctx, int width, int height);

    bool _design_time = true;
    Glib::Property<Glib::ustring> _path;

    static GType gtype;

public:
};

} // namespace

#endif //PROPERTY_WIDGET_H
