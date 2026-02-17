//
// Created by Michael Kowalski on 6/28/25.
//

#include "property-widget.h"

namespace Inkscape::UI::Widget {

#define INIT_PROPERTIES \
    _path(*this, "path", {})

PropertyWidget::PropertyWidget() :
    Glib::ObjectBase("PropertyWidget"),
    INIT_PROPERTIES {

    construct();
}

PropertyWidget::PropertyWidget(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder):
    Glib::ObjectBase("PropertyWidget"),
    Gtk::DrawingArea(cobject),
    INIT_PROPERTIES {

    construct();
}

PropertyWidget::PropertyWidget(BaseObjectType* cobject):
    Glib::ObjectBase("PropertyWidget"),
    Gtk::DrawingArea(cobject),
    INIT_PROPERTIES {

    construct();
}

#undef INIT_PROPERTIES

GType PropertyWidget::gtype = 0;

void PropertyWidget::construct() {
    set_draw_func([this](auto& ctx, auto w, auto h) {
        draw_text(ctx, w, h);
    });

    property_path().signal_changed().connect([this]{ queue_draw(); });
}

void PropertyWidget::draw_text(const Cairo::RefPtr<Cairo::Context>& ctx, int width, int height) {
    if (!_design_time) return;

    ctx->select_font_face("Sans", Cairo::ToyFontFace::Slant::NORMAL, Cairo::ToyFontFace::Weight::NORMAL);
    ctx->set_font_size(12);
    ctx->set_source_rgb(0, 0.6, 0);
    ctx->move_to(0, 12);
    ctx->show_text(_path.get_value().raw());
}

} // namespace