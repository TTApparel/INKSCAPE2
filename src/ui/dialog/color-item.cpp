// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @brief Color item used in palettes and swatches UI.
 */
/* Authors: PBS <pbs3141@gmail.com>
 * Copyright (C) 2022 PBS
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "color-item.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <utility>
#include <vector>
#include <cairomm/context.h>
#include <cairomm/pattern.h>
#include <cairomm/surface.h>
#include <glibmm/bytes.h>
#include <glibmm/convert.h>
#include <glibmm/i18n.h>
#include <giomm/menu.h>
#include <giomm/menuitem.h>
#include <giomm/simpleaction.h>
#include <giomm/simpleactiongroup.h>
#include <gdkmm/contentprovider.h>
#include <gdkmm/general.h>
#include <gdkmm/pixbuf.h>
#include <gdkmm/texture.h>
#include <gtkmm/binlayout.h>
#include <gtkmm/dragsource.h>
#include <gtkmm/eventcontrollermotion.h>
#include <gtkmm/gestureclick.h>
#include <gtkmm/popover.h>
#include <gtkmm/popovermenu.h>
#include <sigc++/functors/mem_fun.h>

#include "colors/dragndrop.h"
#include "desktop-style.h"
#include "document.h"
#include "document-undo.h"
#include "message-context.h"
#include "preferences.h"
#include "selection.h"
#include "actions/actions-tools.h"
#include "display/cairo-utils.h"
#include "io/resource.h"
#include "object/sp-gradient.h"
#include "object/tags.h"
#include "ui/containerize.h"
#include "ui/controller.h"
#include "ui/dialog/dialog-base.h"
#include "ui/dialog/dialog-container.h"
#include "ui/icon-names.h"
#include "ui/util.h"
#include "util/value-utils.h"
#include "util/variant-visitor.h"

namespace Inkscape::UI::Dialog {
namespace {

/// Get the "remove-color" image.
Glib::RefPtr<Gdk::Pixbuf> get_removecolor()
{
    // Load the pixbuf only once
    static const auto remove_color = [] {
        auto path = IO::Resource::get_path(IO::Resource::SYSTEM, IO::Resource::UIS, "resources", "remove-color.png");
        auto pixbuf = Gdk::Pixbuf::create_from_file(path.pointer());
        if (!pixbuf) {
            std::cerr << "Null pixbuf for " << Glib::filename_to_utf8(path.pointer()) << std::endl;
        }
        return pixbuf;
    }();
    return remove_color;
}

} // namespace

ColorItem::ColorItem(DialogBase *dialog)
    : dialog(dialog)
{
    data = PaintNone();
    pinned_default = true;
    add_css_class("paint-none");
    description = C_("Paint", "None");
    color_id = "none";
    common_setup();
}

ColorItem::ColorItem(Colors::Color color, DialogBase *dialog)
    : dialog(dialog)
{
    description = color.getName();
    color_id = color_to_id(color);
    data = std::move(color);
    common_setup();
}

ColorItem::ColorItem(SPGradient *gradient, DialogBase *dialog)
    : dialog(dialog)
{
    data = GradientData{gradient};
    description = gradient->defaultLabel();
    color_id = gradient->getId();

    gradient->connectRelease(sigc::track_object([this] (SPObject*) {
        std::get<GradientData>(data).gradient = nullptr;
    }, *this));

    gradient->connectModified(sigc::track_object([this] (auto sender, SPObject *obj, unsigned flags) {
        if (flags & SP_OBJECT_STYLE_MODIFIED_FLAG) {
            cache_dirty = true;
            queue_draw();
        }
        description = obj->defaultLabel();
        _signal_modified.emit();
        if (is_pinned() != was_grad_pinned) {
            was_grad_pinned = is_pinned();
            _signal_pinned.emit();
        }
    }, *this));

    was_grad_pinned = is_pinned();
    common_setup();
}

ColorItem::ColorItem(Glib::ustring name)
    : description(std::move(name))
{
    bool group = !description.empty();
    set_name("ColorItem");
    set_tooltip_text(description);
    color_id = "-";
    add_css_class(group ? "group" : "filler");
}

ColorItem::~ColorItem() = default;

bool ColorItem::is_group() const {
    return !dialog && color_id == "-" && !description.empty();
}

bool ColorItem::is_filler() const {
    return !dialog && color_id == "-" && description.empty();
}

void ColorItem::common_setup()
{
    containerize(*this);
    set_layout_manager(Gtk::BinLayout::create());
    set_name("ColorItem");
    set_tooltip_text(description + (tooltip.empty() ? tooltip : "\n" + tooltip));

    set_draw_func(sigc::mem_fun(*this, &ColorItem::draw_func));

    auto const drag = Gtk::DragSource::create();
    drag->set_button(1); // left
    drag->set_actions(Gdk::DragAction::MOVE | Gdk::DragAction::COPY);
    drag->signal_prepare().connect([this](auto &&...) { return on_drag_prepare(); }, false); // before
    drag->signal_drag_begin().connect([this, &drag = *drag](auto &&...) { on_drag_begin(drag); });
    add_controller(drag);

    auto const motion = Gtk::EventControllerMotion::create();
    motion->set_propagation_phase(Gtk::PropagationPhase::TARGET);
    motion->signal_enter().connect([this](auto &&...) { on_motion_enter(); });
    motion->signal_leave().connect([this](auto &&...) { on_motion_leave(); });
    add_controller(motion);

    auto const click = Gtk::GestureClick::create();
    click->signal_pressed().connect(Controller::use_state([this](auto& controller, auto &&...) { return on_click_pressed(controller); }, *click));
    click->signal_released().connect(Controller::use_state([this](auto& controller, auto &&...) { return on_click_released(controller); }, *click));
    add_controller(click);
}

void ColorItem::set_pinned_pref(const std::string &path)
{
    pinned_pref = path + "/pinned/" + color_id;
}

void ColorItem::draw_color(Cairo::RefPtr<Cairo::Context> const &cr, int w, int h) const
{
    std::visit(VariantVisitor{
    [&] (Undefined) {
        // there's no color to paint; indicate clearly that there is nothing to select:
        auto y = h / 2 + 0.5;
        auto width = w / 4;
        auto x = (w - width) / 2 - 0.5;
        cr->move_to(x, y);
        cr->line_to(x + width, y);
        auto const fg = get_color();
        cr->set_source_rgba(fg.get_red(), fg.get_green(), fg.get_blue(), 0.5);
        cr->set_line_width(1);
        cr->stroke();
    },
    [&] (PaintNone) {
        if (auto const pixbuf = get_removecolor()) {
            const auto device_scale = get_scale_factor();
            cr->save();
            cr->scale((double)w / pixbuf->get_width() / device_scale, (double)h / pixbuf->get_height() / device_scale);
            Gdk::Cairo::set_source_pixbuf(cr, pixbuf, 0, 0);
            cr->paint();
            cr->restore();
        }
    },
    [&] (Colors::Color const &color) {
        ink_cairo_set_source_color(cr, color);
        cr->paint();
        // there's no way to query background color to check if color item stands out,
        // so we apply faint outline to let users make out color shapes blending with background
        auto const fg = get_color();
        cr->rectangle(0.5, 0.5, w - 1, h - 1);
        cr->set_source_rgba(fg.get_red(), fg.get_green(), fg.get_blue(), 0.07);
        cr->set_line_width(1);
        cr->stroke();
    },
    [&] (GradientData graddata) {
        // Gradient pointer may be null if the gradient was destroyed.
        auto grad = graddata.gradient;
        if (!grad) return;

        auto pat_checkerboard = Cairo::RefPtr<Cairo::Pattern>(new Cairo::Pattern(ink_cairo_pattern_create_checkerboard(), true));
        auto pat_gradient     = Cairo::RefPtr<Cairo::Pattern>(new Cairo::Pattern(grad->create_preview_pattern(w),         true));

        cr->set_source(pat_checkerboard);
        cr->paint();
        cr->set_source(pat_gradient);
        cr->paint();
    }}, data);
}

void ColorItem::draw_func(Cairo::RefPtr<Cairo::Context> const &cr, int const w, int const h)
{
    // Only using caching for none and gradients. None is included because the image is huge.
    bool const use_cache = std::holds_alternative<PaintNone>(data) || std::holds_alternative<GradientData>(data);

    if (use_cache) {
        auto scale = get_scale_factor();
        // Ensure cache exists and has correct size.
        if (!cache || cache->get_width() != w * scale || cache->get_height() != h * scale) {
            cache = Cairo::ImageSurface::create(Cairo::Surface::Format::ARGB32, w * scale, h * scale);
            cairo_surface_set_device_scale(cache->cobj(), scale, scale);
            cache_dirty = true;
        }
        // Ensure cache contents is up-to-date.
        if (cache_dirty) {
            draw_color(Cairo::Context::create(cache), w * scale, h * scale);
            cache_dirty = false;
        }
        // Paint from cache.
        cr->set_source(cache, 0, 0);
        cr->paint();
    } else {
        // Paint directly.
        draw_color(cr, w, h);
    }

    // Draw fill/stroke indicators.
    if (is_fill || is_stroke) {
        double const lightness = Colors::get_perceptual_lightness(getColor());
        auto [gray, alpha] = Colors::get_contrasting_color(lightness);
        cr->set_source_rgba(gray, gray, gray, alpha);

        // Scale so that the square -1...1 is the biggest possible square centred in the widget.
        auto minwh = std::min(w, h);
        cr->translate((w - minwh) / 2.0, (h - minwh) / 2.0);
        cr->scale(minwh / 2.0, minwh / 2.0);
        cr->translate(1.0, 1.0);

        if (is_fill) {
            cr->arc(0.0, 0.0, 0.35, 0.0, 2 * M_PI);
            cr->fill();
        }

        if (is_stroke) {
            cr->set_fill_rule(Cairo::Context::FillRule::EVEN_ODD);
            cr->arc(0.0, 0.0, 0.65, 0.0, 2 * M_PI);
            cr->arc(0.0, 0.0, 0.5, 0.0, 2 * M_PI);
            cr->fill();
        }
    }
}

void ColorItem::size_allocate_vfunc(int width, int height, int baseline)
{
    Gtk::DrawingArea::size_allocate_vfunc(width, height, baseline);

    cache_dirty = true;
}

void ColorItem::on_motion_enter()
{
    assert(dialog);

    mouse_inside = true;
    if (auto desktop = dialog->getDesktop()) {
        auto msg = Glib::ustring::compose(_("Color: <b>%1</b>; <b>Click</b> to set fill, <b>Shift+click</b> to set stroke"), description);
        desktop->tipsMessageContext()->set(Inkscape::INFORMATION_MESSAGE, msg.c_str());
    }
}

void ColorItem::on_motion_leave()
{
    assert(dialog);

    mouse_inside = false;
    if (auto desktop = dialog->getDesktop()) {
        desktop->tipsMessageContext()->clear();
    }
}

Gtk::EventSequenceState ColorItem::on_click_pressed(Gtk::GestureClick const &click)
{
    assert(dialog);

    if (click.get_current_button() == 3) {
        on_rightclick();
        return Gtk::EventSequenceState::CLAIMED;
    }
    // Return true necessary to avoid stealing the canvas focus.
    return Gtk::EventSequenceState::CLAIMED;
}

Gtk::EventSequenceState ColorItem::on_click_released(Gtk::GestureClick const &click)
{
    assert(dialog);

    auto const button = click.get_current_button();
    if (mouse_inside && (button == 1 || button == 2)) {
        auto const state = click.get_current_event_state();
        auto const stroke = button == 2 || Controller::has_flag(state, Gdk::ModifierType::SHIFT_MASK);
        on_click(stroke);
        return Gtk::EventSequenceState::CLAIMED;
    }
    return Gtk::EventSequenceState::NONE;
}

void ColorItem::on_click(bool stroke)
{
    assert(dialog);

    auto desktop = dialog->getDesktop();
    if (!desktop) return;

    auto attr_name = stroke ? "stroke" : "fill";
    auto css = std::unique_ptr<SPCSSAttr, void(*)(SPCSSAttr*)>(sp_repr_css_attr_new(), [] (auto p) {sp_repr_css_attr_unref(p);});

    Glib::ustring descr;
    if (is_paint_none()) {
        sp_repr_css_set_property(css.get(), attr_name, "none");
        descr = stroke ? _("Set stroke color to none") : _("Set fill color to none");
    } else if (auto const color = std::get_if<Colors::Color>(&data)) {
        sp_repr_css_set_property_string(css.get(), attr_name, color->toString());
        descr = stroke ? _("Set stroke color from swatch") : _("Set fill color from swatch");
    } else if (auto const graddata = std::get_if<GradientData>(&data)) {
        auto grad = graddata->gradient;
        if (!grad) return;
        auto colorspec = "url(#" + Glib::ustring(grad->getId()) + ")";
        sp_repr_css_set_property(css.get(), attr_name, colorspec.c_str());
        descr = stroke ? _("Set stroke color from swatch") : _("Set fill color from swatch");
    }

    sp_desktop_set_style(desktop, css.get());

    DocumentUndo::done(desktop->getDocument(), descr.c_str(), INKSCAPE_ICON("swatches"));
}

void ColorItem::on_rightclick()
{
    // Only re/insert actions on click, not in ctor, to avoid performance hit on rebuilding palette
    auto const main_actions = Gio::SimpleActionGroup::create();
    main_actions->add_action("set-fill"  , sigc::mem_fun(*this, &ColorItem::action_set_fill  ));
    main_actions->add_action("set-stroke", sigc::mem_fun(*this, &ColorItem::action_set_stroke));
    main_actions->add_action("delete"    , sigc::mem_fun(*this, &ColorItem::action_delete    ));
    main_actions->add_action("edit"      , sigc::mem_fun(*this, &ColorItem::action_edit      ));
    main_actions->add_action("toggle-pin", sigc::mem_fun(*this, &ColorItem::action_toggle_pin));
    insert_action_group("color-item", main_actions);

    auto const menu = Gio::Menu::create();

    // TRANSLATORS: An item in context menu on a colour in the swatches
    menu->append(_("Set Fill"  ), "color-item.set-fill"  );
    menu->append(_("Set Stroke"), "color-item.set-stroke");

    auto section = menu;

    if (auto const graddata = std::get_if<GradientData>(&data)) {
        section = Gio::Menu::create();
        menu->append_section(section);
        section->append(_("Delete" ), "color-item.delete");
        section->append(_("Edit..."), "color-item.edit"  );
        section = Gio::Menu::create();
        menu->append_section(section);
    }

    section->append(is_pinned() ? _("Unpin Color") : _("Pin Color"), "color-item.toggle-pin");

    // If document has gradients, add Convert section w/ actions to convert them to swatches.
    auto grad_names = std::vector<Glib::ustring>{};
    for (auto const obj : dialog->getDesktop()->getDocument()->getResourceList("gradient")) {
        auto const grad = static_cast<SPGradient *>(obj);
        if (grad->hasStops() && !grad->isSwatch()) {
            grad_names.emplace_back(grad->getId());
        }
    }
    if (!grad_names.empty()) {
        auto const convert_actions = Gio::SimpleActionGroup::create();
        auto const convert_submenu = Gio::Menu::create();

        std::sort(grad_names.begin(), grad_names.end());
        for (auto const &name: grad_names) {
            convert_actions->add_action(name, sigc::bind(sigc::mem_fun(*this, &ColorItem::action_convert), name));
            convert_submenu->append(name, "color-item-convert." + name);
        }

        insert_action_group("color-item-convert", convert_actions);

        section = Gio::Menu::create();
        section->append_submenu(_("Convert"), convert_submenu);
        menu->append_section(section);
    }


    if (_popover) {
        _popover->unparent();
    }

    _popover = std::make_unique<Gtk::PopoverMenu>(menu, Gtk::PopoverMenu::Flags::NESTED);
    _popover->set_parent(*this);

    _popover->popup();
}

void ColorItem::action_set_fill()
{
    on_click(false);
}

void ColorItem::action_set_stroke()
{
    on_click(true);
}

void ColorItem::action_delete()
{
    auto const grad = std::get<GradientData>(data).gradient;
    if (!grad) return;

    grad->setSwatch(false);
    DocumentUndo::done(grad->document, _("Delete swatch"), INKSCAPE_ICON("color-gradient"));
}

void ColorItem::action_edit()
{
    auto const grad = std::get<GradientData>(data).gradient;
    if (!grad) return;

    auto desktop = dialog->getDesktop();
    auto selection = desktop->getSelection();
    auto items = std::vector<SPItem*>(selection->items().begin(), selection->items().end());

    if (!items.empty()) {
        auto query = SPStyle(desktop->doc());
        int result = objects_query_fillstroke(items, &query, true);
        if (result == QUERY_STYLE_MULTIPLE_SAME || result == QUERY_STYLE_SINGLE) {
            if (query.fill.isPaintserver()) {
                if (cast<SPGradient>(query.getFillPaintServer()) == grad) {
                    desktop->getContainer()->new_dialog("FillStroke");
                    return;
                }
            }
        }
    }

    // Otherwise, invoke the gradient tool.
    set_active_tool(desktop, "Gradient");
}

void ColorItem::action_toggle_pin()
{
    if (auto const graddata = std::get_if<GradientData>(&data)) {
        auto const grad = graddata->gradient;
        if (!grad) return;

        grad->setPinned(!is_pinned());
        DocumentUndo::done(grad->document, is_pinned() ? _("Pin swatch") : _("Unpin swatch"), INKSCAPE_ICON("color-gradient"));
    } else {
        Inkscape::Preferences::get()->setBool(pinned_pref, !is_pinned());
    }
}

void ColorItem::action_convert(Glib::ustring const &name)
{
    // This will not be needed until next menu
    remove_action_group("color-item-convert");

    auto const doc = dialog->getDesktop()->getDocument();
    auto const resources = doc->getResourceList("gradient");
    auto const it = std::find_if(resources.cbegin(), resources.cend(),
                                 [&](auto &_){ return _->getId() == name; });
    if (it == resources.cend()) return;

    auto const grad = static_cast<SPGradient *>(*it);
    grad->setSwatch();
    DocumentUndo::done(doc, _("Add gradient stop"), INKSCAPE_ICON("color-gradient"));
}

Glib::RefPtr<Gdk::ContentProvider> ColorItem::on_drag_prepare()
{
    if (!dialog) return {};

    Colors::Paint paint;
    if (!is_paint_none()) {
        paint = getColor();
    }

    return Gdk::ContentProvider::create(Util::GlibValue::create<Colors::Paint>(std::move(paint)));
}

void ColorItem::on_drag_begin(Gtk::DragSource &source)
{
    constexpr int w = 32;
    constexpr int h = 24;

    auto surface = Cairo::ImageSurface::create(Cairo::Surface::Format::ARGB32, w, h);
    draw_color(Cairo::Context::create(surface), w, h);
    auto const pixbuf = Gdk::Pixbuf::create(surface, 0, 0, w, h);
    auto const texture = Gdk::Texture::create_for_pixbuf(pixbuf);
    source.set_icon(texture, 0, 0);
}

void ColorItem::set_fill(bool b)
{
    is_fill = b;
    queue_draw();
}

void ColorItem::set_stroke(bool b)
{
    is_stroke = b;
    queue_draw();
}

bool ColorItem::is_pinned() const
{
    if (auto const graddata = std::get_if<GradientData>(&data)) {
        auto const grad = graddata->gradient;
        return grad && grad->isPinned();
    } else {
        return Inkscape::Preferences::get()->getBool(pinned_pref, pinned_default);
    }
}

/**
 * Return the average color for this color item. If none, returns white
 * but if a gradient an average of the gradient in RGB is returned.
 */
Colors::Color ColorItem::getColor() const
{
    return std::visit(VariantVisitor{
    [] (Undefined) {
        assert(false);
        return Colors::Color{0xffffffff};
    },
    [] (PaintNone) {
        return Colors::Color(0xffffffff);
    },
    [] (Colors::Color const &color) {
        return color;
    },
    [] (GradientData graddata) {
        auto grad = graddata.gradient;
        assert(grad);
        auto pat = Cairo::RefPtr<Cairo::Pattern>(new Cairo::Pattern(grad->create_preview_pattern(1), true));
        auto img = Cairo::ImageSurface::create(Cairo::Surface::Format::ARGB32, 1, 1);
        auto cr = Cairo::Context::create(img);
        cr->set_source(pat);
        cr->paint();
        auto color = ink_cairo_surface_average_color(img->cobj());
        color.setName(grad->getId());
        return color;
    }}, data);
}

bool ColorItem::is_paint_none() const {
    return std::holds_alternative<PaintNone>(data);
}


} // namespace Inkscape::UI::Dialog

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
