// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * A slider with colored background - implementation.
 *//*
 * Authors:
 *   see git history
 *
 * Copyright (C) 2018-2024 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "color-slider.h"

#include <cairomm/pattern.h>
#include <gdkmm/general.h>
#include <gtkmm/drawingarea.h>
#include <gtkmm/eventcontrollermotion.h>
#include <gtkmm/gestureclick.h>
#include <sigc++/functors/mem_fun.h>
#include <utility>

#include "colors/color.h"
#include "colors/color-set.h"
#include "colors/spaces/base.h"
#include "colors/spaces/components.h"
#include "preferences.h"
#include "ui/controller.h"
#include "ui/util.h"
#include "util/drawing-utils.h"
#include "util/theme-utils.h"

static constexpr int THUMB_SPACE = 16;
static constexpr int THUMB_SIZE = 10;
constexpr uint32_t ERR_DARK = 0xff00ff00;    // Green
constexpr uint32_t ERR_LIGHT = 0xffff00ff;   // Magenta

namespace Inkscape::UI::Widget {

ColorSlider::ColorSlider(
    BaseObjectType *cobject,
    Glib::RefPtr<Gtk::Builder> const &builder,
    std::shared_ptr<Colors::ColorSet> colors,
    Colors::Space::Component component)
    : Gtk::DrawingArea(cobject)
    , _colors(std::move(colors))
    , _component(std::move(component))
{
    set_name("ColorSlider");

    set_draw_func(sigc::mem_fun(*this, &ColorSlider::draw_func));

    auto const click = Gtk::GestureClick::create();
    click->set_button(1); // left
    click->signal_pressed().connect([this, &click = *click](auto &&...args) { on_click_pressed(click, args...); });
    add_controller(click);

    auto const motion = Gtk::EventControllerMotion::create();
    motion->signal_motion().connect([this, &motion = *motion](auto &&...args) { on_motion(motion, args...); });
    add_controller(motion);

    _changed_connection = _colors->signal_changed.connect([this]() {
        queue_draw();
    });
}

static Geom::OptIntRect get_active_area(int full_width, int full_height) {
    int width = full_width - THUMB_SPACE;
    if (width <= 0) return {};

    int left = THUMB_SPACE / 2;
    int top = 0;
    return Geom::IntRect::from_xywh(left, top, width, full_height);
}

static double get_value_at(Gtk::Widget const &self, double const x, double const y)
{
    auto area = get_active_area(self.get_width(), self.get_height());
    if (!area) return 0.0;
    return CLAMP((x - area->left()) / area->width(), 0.0, 1.0);
}

void ColorSlider::on_click_pressed(Gtk::GestureClick const &click,
                                   int /*n_press*/, double const x, double const y)
{
    update_component(x, y, click.get_current_event_state());
}

void ColorSlider::on_motion(Gtk::EventControllerMotion const &motion, double x, double y)
{
    auto const state = motion.get_current_event_state();
    if (Controller::has_flag(state, Gdk::ModifierType::BUTTON1_MASK)) {
        // only update color if user is dragging the slider;
        // don't rely on any click/release events, as release event might be lost leading to unintended updates
        update_component(x, y, state);
    }
}

void ColorSlider::update_component(double x, double y, Gdk::ModifierType const state)
{
    // auto const constrained = Controller::has_flag(state, Gdk::ModifierType::CONTROL_MASK);
    // XXX We don't know how to deal with constraints yet.
    if (_colors->isValid(_component) && _colors->setAll(_component, get_value_at(*this, x, y))) {
        signal_value_changed.emit();
    }
}

/**
 * Generate a checkerboard pattern with the given colors.
 *
 * @arg dark - The RGBA dark color
 * @arg light - The RGBA light color
 * @arg scale - The scale factor of the cairo surface
 * @arg buffer - The memory to populate with this pattern
 *
 * @returns A Gdk::Pixbuf of the buffer memory.
 */
Glib::RefPtr<Gdk::Pixbuf> _make_checkerboard(uint32_t dark, uint32_t light, unsigned scale, std::vector<uint32_t> &buffer)
{
    // A pattern of 2x2 blocks is enough for REPEAT mode to do the rest, this way we never need to recalculate the checkerboard
    static auto block = 0x09 * scale;
    static auto pattern = block * 2;

    buffer = std::vector<uint32_t>(pattern * pattern);
    for (auto y = 0; y < pattern; y++) {
        for (auto x = 0; x < pattern; x++) {
            buffer[(y * pattern) + x] = ((x / block) & 1) != ((y / block) & 1) ? dark : light;
        }
    }
    return Gdk::Pixbuf::create_from_data((guint8*)buffer.data(), Gdk::Colorspace::RGB, true, 8, pattern, pattern, pattern * 4);
}

static void draw_slider_thumb(const Cairo::RefPtr<Cairo::Context>& ctx, const Geom::Point& location, double size, const Gdk::RGBA& fill, const Gdk::RGBA& stroke, int device_scale) {
    auto center = location.round(); //todo - verify pix grid fit + Geom::Point(0.5, 0.5);
    auto radius = size / 2;
    auto alpha = 0.06 / device_scale;
    double step = 1.0 / device_scale;
    for (int i = 2 * device_scale; i > 0; --i) {
        ctx->set_source_rgba(0, 0, 0, alpha);
        alpha *= 1.5;
        auto offset = Geom::Point{1, 1} * step * i;
        ctx->arc(center.x() + offset.x(), center.y() + offset.y(), radius+1, 0, 2 * M_PI);
        ctx->fill();
    }
    // border/outline
    ctx->arc(center.x(), center.y(), radius+1, 0, 2 * M_PI);
    ctx->set_source_rgb(stroke.get_red(), stroke.get_green(), stroke.get_blue());
    ctx->fill();
    // fill
    ctx->arc(center.x(), center.y(), radius, 0, 2 * M_PI);
    ctx->set_source_rgb(fill.get_red(), fill.get_green(), fill.get_blue());
    ctx->fill();
}

void ColorSlider::draw_func(Cairo::RefPtr<Cairo::Context> const &cr,
                            int const full_width, int const full_height)
{
    auto maybe_area = get_active_area(full_width, full_height);
    if (!maybe_area) return;

    auto area = *maybe_area;
    bool dark_theme = Util::is_current_theme_dark(*this);

    // expand border past active area on both sides, so slider's thumb doesn't hang at any extreme, but looks confined
    auto border = area;
    border.expandBy(1, 0);
    double radius = 3;
    Util::rounded_rectangle(cr, border, radius);

    auto const scale = get_scale_factor();
    auto width = border.width() * scale;
    // auto height = area->height() * scale;
    auto left = border.left() * scale;
    auto top = border.top() * scale;
    bool const is_alpha = _component.id == "a";

    // changing scale to draw pixmap at display resolution
    cr->save();
    cr->scale(1.0 / scale, 1.0 / scale);

    // Color set is empty, this is not allowed, show warning colors
    if (_colors->isEmpty()) {
        static std::vector<uint32_t> err_buffer;
        static Glib::RefPtr<Gdk::Pixbuf> error = _make_checkerboard(ERR_DARK, ERR_LIGHT, scale, err_buffer);

        Gdk::Cairo::set_source_pixbuf(cr, error, left, top);
        cr->get_source()->set_extend(Cairo::Pattern::Extend::REPEAT);
        cr->fill();

        // Don't try and paint any color (there isn't any)
        cr->restore();
        return;
    }

    // The alpha background is a checkerboard pattern of light and dark pixels
    if (is_alpha) {
        std::vector<uint32_t> bg_buffer;
        auto [col1, col2] = Util::get_checkerboard_colors(*this);
        Glib::RefPtr<Gdk::Pixbuf> background = _make_checkerboard(col1, col2, scale, bg_buffer);

        // Paint the alpha background
        Gdk::Cairo::set_source_pixbuf(cr, background, left, top);
        cr->get_source()->set_extend(Cairo::Pattern::Extend::REPEAT);
        cr->fill_preserve();
    }

    // Draw row of colored pixels here
    auto paint_color = _colors->getAverage();

    if (!is_alpha) {
        // Remove alpha channel from paint
        paint_color.enableOpacity(false);
    }

    // When the widget is wider, we want a new color gradient buffer
    if (!_gradient || _gr_buffer.size() < static_cast<size_t>(width)) {
        _gr_buffer.resize(width);
        _gradient = Gdk::Pixbuf::create_from_data((guint8*)&_gr_buffer.front(), Gdk::Colorspace::RGB, true, 8, width, 1, width * 4);
    }

    double lim = width > 1 ? width - 1.0 : 1.0;
    for (int x = 0; x < width; x++) {
        paint_color.set(_component.index, x / lim);
        _gr_buffer[x] = paint_color.toABGR();
    }

    Gdk::Cairo::set_source_pixbuf(cr, _gradient, left, top);
    cr->get_source()->set_extend(Cairo::Pattern::Extend::REPEAT);
    cr->fill();
    cr->restore();

    Util::draw_standard_border(cr, border, dark_theme, radius, scale);

    // draw slider thumb
    auto style = get_style_context();
    auto fill = Util::lookup_background_color(style);
    if (!dark_theme || !fill) {
        float x = dark_theme ? 0.3f : 1.0f;
        fill = Gdk::RGBA(x, x, x);
    }
    auto stroke = Util::lookup_foreground_color(style);
    if (!stroke) {
        float x = dark_theme ? 0.9f : 0.3f;
        stroke = Gdk::RGBA(x, x, x);
    }
    if (_colors->isValid(_component)) {
        double value = _colors->getAverage(_component);
        draw_slider_thumb(cr, Geom::Point(area.left() + value * area.width(), area.midpoint().y()), THUMB_SIZE, *fill, *stroke, get_scale_factor());
    }
}

double ColorSlider::getScaled() const
{
    if (_colors->isEmpty())
        return 0.0;
    return _colors->getAverage(_component) * _component.scale;
}

void ColorSlider::setScaled(double value)
{
    if (!_colors->isValid(_component)) {
        g_message("ColorSlider - cannot set color channel, it is not valid.");
        return;
    }
    // setAll replaces every color with the same value, setAverage moves them all by the same amount.
    _colors->setAll(_component, value / _component.scale);
}

} // namespace Inkscape::UI::Widget

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
