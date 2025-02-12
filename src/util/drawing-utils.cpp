// SPDX-License-Identifier: GPL-2.0-or-later

#include "drawing-utils.h"

namespace Inkscape::Util {

Geom::Rect rounded_rectangle(const Cairo::RefPtr<Cairo::Context>& ctx, const Geom::Rect& rect, double radius) {
    auto x = rect.left();
    auto y = rect.top();
    auto width = rect.width();
    auto height = rect.height();
    if (radius > 0) {
        ctx->arc(x + width - radius, y + radius, radius, -M_PI_2, 0);
        ctx->arc(x + width - radius, y + height - radius, radius, 0, M_PI_2);
        ctx->arc(x + radius, y + height - radius, radius, M_PI_2, M_PI);
        ctx->arc(x + radius, y + radius, radius, M_PI, 3 * M_PI_2);
        ctx->close_path();
    }
    else {
        ctx->move_to(x, y);
        ctx->line_to(x + width, y);
        ctx->line_to(x + width, y + width);
        ctx->line_to(x, y + width);
        ctx->close_path();
    }
    return rect.shrunkBy(1);
}

void circle(const Cairo::RefPtr<Cairo::Context>& ctx, const Geom::Point& center, double radius) {
    ctx->arc(center.x(), center.y(), radius, 0, 2 * M_PI);
}

// draw relief around the given rect to stop colors inside blend with background outside
void draw_border(const Cairo::RefPtr<Cairo::Context>& ctx, Geom::Rect rect, double radius, const Gdk::RGBA& color, int device_scale, bool circular) {
    if (rect.width() < 1 || rect.height() < 1) return;

    if (device_scale > 1) {
        // there's one physical pixel overhang on high-dpi display, so eliminate that:
        auto pix = 1.0 / device_scale;
        rect = Geom::Rect::from_xywh(rect.min().x(), rect.min().y(), rect.width() - pix, rect.height() - pix);
    }
    ctx->save();
    // operate on physical pixels
    ctx->scale(1.0 / device_scale, 1.0 / device_scale);
    // align 1.0 wide stroke to pixel grid
    ctx->translate(0.5, 0.5);
    ctx->set_line_width(1.0);
    radius *= device_scale;
    // shadow depth
    const int steps = 3 * device_scale;
    auto alpha = color.get_alpha();
    ctx->set_operator(Cairo::Context::Operator::OVER);
    // rect in physical pixels
    rect = Geom::Rect(rect.min() * device_scale, rect.max() * device_scale);
    for (int i = 0; i < steps; ++i) {
        if (circular) {
            circle(ctx, rect.midpoint(), rect.minExtent() / 2);
            rect.shrinkBy(1);
        }
        else {
            rect = rounded_rectangle(ctx, rect, radius--);
        }
        ctx->set_source_rgba(color.get_red(), color.get_green(), color.get_blue(), alpha);
        ctx->stroke();
        alpha *= 0.5;
    }
    ctx->restore();
}

void draw_standard_border(const Cairo::RefPtr<Cairo::Context>& ctx, Geom::Rect rect, bool dark_theme, double radius, int device_scale, bool circular) {
    auto color = dark_theme ? Gdk::RGBA(1, 1, 1, 0.25) : Gdk::RGBA(0, 0, 0, 0.25);
    draw_border(ctx, rect, radius, color, device_scale, circular);
}

std::optional<Gdk::RGBA> lookup_background_color(Glib::RefPtr<Gtk::StyleContext>& style) {
    Gdk::RGBA color;
    if (style && style->lookup_color("theme_bg_color", color)) {
        return color;
    }
    return {};
}

std::optional<Gdk::RGBA> lookup_foreground_color(Glib::RefPtr<Gtk::StyleContext>& style) {
    Gdk::RGBA color;
    if (style && style->lookup_color("theme_fg_color", color)) {
        return color;
    }
    return {};
}

}
