// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <array>
#include <glibmm/main.h>
#include <glibmm/priorities.h>
#include <glibmm/ustring.h>
#include <giomm/menu.h>
#include <giomm/simpleactiongroup.h>
#include <gtkmm/cellrenderertext.h>
#include <gtkmm/enums.h>
#include <gtkmm/label.h>
#include <gtkmm/progressbar.h>
#include <gtkmm/separator.h>
#include <gtkmm/stringlist.h>
#include <gtkmm/treeiter.h>
#include <gtkmm/widget.h>
#include <iostream>
#include <iterator>
#include <iomanip>
#include <memory>
#include <pangomm/fontdescription.h>
#include <pangomm/fontfamily.h>
#include <set>
#include <glibmm/markup.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/menubutton.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/searchentry2.h>
#include <gtkmm/snapshot.h>
#include <gtkmm/scale.h>
#include <gtkmm/togglebutton.h>
#include <glibmm/i18n.h>
#include <pangomm.h>
#include <libnrtype/font-factory.h>
#include <libnrtype/font-instance.h>
#include <vector>
#include "font-list.h"
#include "preferences.h"
#include "ui/builder-utils.h"
#include "ui/icon-loader.h"
#include "ui/widget/popover-menu-item.h"
#include "ui/widget/popover-menu.h"
#include "util/font-collections.h"
#include "svg/css-ostringstream.h"

using Inkscape::UI::create_builder;

namespace Inkscape::UI::Widget {

std::unique_ptr<FontSelectorInterface> FontList::create_font_list(Glib::ustring path) {
    return std::make_unique<FontList>(path);
}

struct FontListColumnModel : public Gtk::TreeModelColumnRecord {
    // font metadata for installed fonts only
    Gtk::TreeModelColumn<Inkscape::FontInfo> font;
    // font's class
    // Gtk::TreeModelColumn<FontClass> font_class;
    Gtk::TreeModelColumn<bool> injected;
    // fontspec for fonts that are not installed, but used in a document
    Gtk::TreeModelColumn<Glib::ustring> alt_fontspec;
    // icon to show next to a font name (if any)
    Gtk::TreeModelColumn<Glib::ustring> icon_name;

    FontListColumnModel() {
        add(alt_fontspec);
        add(injected);
        add(icon_name);
        add(font);
    }
};

FontListColumnModel g_column_model; // model for font list

// list of font sizes for a slider; combo box has its own list
static std::array<int, 38> g_font_sizes = {
    4, 5, 6, 7, 8, 9, 10, 12, 14, 16, 18, 20, 24, 28, 32, 36,
    44, 56, 64, 72, 80, 96, 112, 128, 144, 160, 192, 224, 256,
    300, 350, 400, 450, 500, 550, 600, 700, 800
};

static int index_to_font_size(int index) {
    if (index < 0) {
        return g_font_sizes.front();
    }
    else if (index >= g_font_sizes.size()) {
        return g_font_sizes.back();
    }
    else {
        return g_font_sizes[index];
    }
}

static int font_size_to_index(double size) {
    auto it = std::lower_bound(begin(g_font_sizes), end(g_font_sizes), static_cast<int>(size));
    return std::distance(begin(g_font_sizes), it);
}

// construct font name from Pango face and family;
// return font name as it is recorded in the font itself, as far as Pango allows it
Glib::ustring get_full_name(const Inkscape::FontInfo& font_info) {
    return get_full_font_name(font_info.ff, font_info.face);
}

Glib::ustring get_alt_name(const Glib::ustring& fontspec) {
    static Glib::ustring sans = "sans-serif";
    if (fontspec.find(sans) != Glib::ustring::npos) {
        auto end = fontspec[sans.size()];
        if (end == 0 || end == ' ' || end == ',') {
            return _("Sans Serif") + fontspec.substr(sans.size());
        }
    }
    return fontspec; // use font spec verbatim
}

class CellFontRenderer : public Gtk::CellRendererText {
public:
    CellFontRenderer() { }

    ~CellFontRenderer() override = default;

    Gtk::Widget* _tree = nullptr;
    bool _show_font_name = true;
    int _font_size = 200; // size in %, where 100 is normal UI font size
    Glib::ustring _sample_text; // text to render (font preview)
    Glib::ustring _name;

    void snapshot_vfunc(Glib::RefPtr<Gtk::Snapshot> const &snapshot, Gtk::Widget &widget, Gdk::Rectangle const &background_area, Gdk::Rectangle const &cell_area, Gtk::CellRendererState flags) override;
};

CellFontRenderer& get_renderer(Gtk::CellRenderer& renderer) {
    return dynamic_cast<CellFontRenderer&>(renderer);
}

Glib::ustring get_font_name(Gtk::TreeIter<Gtk::TreeRow>& iter) {
    if (!iter) return Glib::ustring();

    const Inkscape::FontInfo& font = (*iter)[g_column_model.font];
    auto present = !!font.ff;
    Glib::ustring&& alt = (*iter)[g_column_model.alt_fontspec];
    auto name = Glib::Markup::escape_text(present ? get_full_name(font) : get_alt_name(alt));
    return name;
}

void get_cell_data_func(Gtk::CellRenderer* cell_renderer, Gtk::TreeModel::ConstRow row) {
    auto& renderer = get_renderer(*cell_renderer);

    const Inkscape::FontInfo& font = row[g_column_model.font];
    // auto font_class = row[g_column_model.font_class];
    auto present = !!font.ff;
    Glib::ustring&& icon_name = row[g_column_model.icon_name];
    Glib::ustring&& alt = row[g_column_model.alt_fontspec];

    auto name = Glib::Markup::escape_text(present ? get_full_name(font) : get_alt_name(alt));
    // if no sample text given, then render font name
    auto text = Glib::Markup::escape_text(renderer._sample_text.empty() ? name : renderer._sample_text);

    auto font_desc = Glib::Markup::escape_text(
        present ? Inkscape::get_font_description(font.ff, font.face).to_string() : (alt.empty() ? "sans-serif" : alt));
    auto markup = Glib::ustring::format("<span allow_breaks='false' size='", renderer._font_size, "%' font='", font_desc, "'>", text, "</span>");

    if (renderer._show_font_name) {
        renderer._name = name;
    }

    renderer.set_property("markup", markup);
}

void CellFontRenderer::snapshot_vfunc(Glib::RefPtr<Gtk::Snapshot> const &snapshot, Gtk::Widget &widget, Gdk::Rectangle const &background_area, Gdk::Rectangle const &cell_area, Gtk::CellRendererState flags) {
    Gdk::Rectangle bgnd(background_area);
    Gdk::Rectangle area(cell_area);
    auto margin = 0; // extra space for icon?
    bgnd.set_x(bgnd.get_x() + margin);
    area.set_x(area.get_x() + margin);
    const auto name_font_size = 10; // attempt to select <small> text size
    const auto bottom = area.get_y() + area.get_height(); // bottom where the info font name will be placed
    Glib::RefPtr<Pango::Layout> layout;
    int text_height = 0;

    if (_show_font_name) {
        layout = _tree->create_pango_layout(_name);
        Pango::FontDescription font("Noto"); // wide range of character support
        font.set_weight(Pango::Weight::NORMAL);
        font.set_size(name_font_size * PANGO_SCALE);
        layout->set_font_description(font);
        int text_width = 0;
        // get the text dimensions
        layout->get_pixel_size(text_width, text_height);
        // shrink area to prevent overlap
        area.set_height(area.get_height() - text_height);
    }

    CellRendererText::snapshot_vfunc(snapshot, widget, bgnd, area, flags);

    if (_show_font_name) {
        auto context = _tree->get_style_context();
        Gtk::StateFlags sflags = _tree->get_state_flags();
        if ((bool)(flags & Gtk::CellRendererState::SELECTED)) {
            sflags |= Gtk::StateFlags::SELECTED;
        }
        context->set_state(sflags);
        Gdk::RGBA fg = context->get_color();
        snapshot->save();
        snapshot->translate({(float)(area.get_x() + 2), (float)bottom - text_height});
        snapshot->append_layout(layout, fg);
        snapshot->restore();
    }
}

const char* get_sort_icon(Inkscape::FontOrder order) {
    const char* icon = nullptr;

    switch (order) {
        case Inkscape::FontOrder::by_name:
            icon = "sort-alphabetically-symbolic";
            break;

        case Inkscape::FontOrder::by_weight:
            icon = "sort-by-weight-symbolic";
            break;

        case Inkscape::FontOrder::by_width:
            icon = "sort-by-width-symbolic";
            break;

        default:
            g_warning("Missing case in get_sort_icon");
            break;
    }

    return icon;
}

void set_grid_cell_size(Gtk::CellRendererText* renderer, int font_size_percent) {
    // TODO: use pango layout to calc sizes
    int size = 20 * font_size_percent / 100;
    renderer->set_fixed_size(size * 4 / 3, size);
};

FontList::FontList(Glib::ustring preferences_path) :
    _prefs(std::move(preferences_path)),
    _builder(create_builder("font-list.glade")),
    _main_grid(get_widget<Gtk::Grid>(_builder, "main-grid")),
    _tag_list(get_widget<Gtk::ListBox>(_builder, "categories")),
    _font_list(get_widget<Gtk::TreeView>(_builder, "font-list")),
    _font_grid(get_widget<Gtk::IconView>(_builder, "font-grid")),
    _font_size(get_widget<Gtk::ComboBoxText>(_builder, "font-size")),
    _font_size_scale(get_widget<Gtk::Scale>(_builder, "font-size-scale")),
    _tag_box(get_widget<Gtk::Box>(_builder, "tag-box")),
    _info_box(get_widget<Gtk::Box>(_builder, "info-box")),
    _progress_box(get_widget<Gtk::Box>(_builder, "progress-box")),
    _font_tags(Inkscape::FontTags::get())
{
    _cell_renderer = std::make_unique<CellFontRenderer>();
    auto font_renderer = static_cast<CellFontRenderer*>(_cell_renderer.get());
    font_renderer->_tree = &_font_list;

    _cell_icon_renderer = std::make_unique<Gtk::CellRendererPixbuf>();
    auto ico = static_cast<Gtk::CellRendererPixbuf*>(_cell_icon_renderer.get());
    ico->set_fixed_size(16, 16);

    _grid_renderer = std::make_unique<CellFontRenderer>();
    auto grid_renderer = static_cast<CellFontRenderer*>(_grid_renderer.get());
    grid_renderer->_show_font_name = false;

    _font_list_store = Gtk::ListStore::create(g_column_model);

    get_widget<Gtk::Box>(_builder, "variants").append(_font_variations);
    _font_variations.get_size_group(0)->add_widget(get_widget<Gtk::Label>(_builder, "font-size-label"));
    _font_variations.get_size_group(1)->add_widget(_font_size);
    _font_variations.connectChanged([this](){
        if (_update.pending()) return;
        _signal_changed.emit();
    });

    set_hexpand();
    set_vexpand();
    append(_main_grid);
    set_margin_start(0);
    set_margin_end(0);
    set_margin_top(5);
    set_margin_bottom(0);

    auto options = &get_widget<Gtk::ToggleButton>(_builder, "btn-options");
    auto options_grid = &get_widget<Gtk::Grid>(_builder, "options-grid");
    options->signal_toggled().connect([=](){
        options_grid->set_visible(options->get_active());
    });

    std::pair<const char*, Inkscape::FontOrder> sorting[3] = {
        {N_("Sort alphabetically"), Inkscape::FontOrder::by_name},
        {N_("Light to heavy"), Inkscape::FontOrder::by_weight},
        {N_("Condensed to expanded"), Inkscape::FontOrder::by_width}
    };
    auto sort_menu = Gtk::make_managed<PopoverMenu>(Gtk::PositionType::BOTTOM);
    for (auto&& el : sorting) {
        auto item = Gtk::make_managed<PopoverMenuItem>();
        auto hbox = Gtk::make_managed<Gtk::Box>();
        hbox->append(*Gtk::manage(sp_get_icon_image(get_sort_icon(el.second), Gtk::IconSize::NORMAL)));
        hbox->append(*Gtk::make_managed<Gtk::Label>(_(el.first)));
        hbox->set_spacing(4);
        item->set_child(*hbox);
        item->signal_activate().connect([=, this] { sort_fonts(el.second); });
        sort_menu->append(*item);
    }
    get_widget<Gtk::MenuButton>(_builder, "btn-sort").set_popover(*sort_menu);
    get_widget<Gtk::Button>(_builder, "id-reset-filter").signal_clicked().connect([this](){
        bool modified = false;
        if (_font_tags.deselect_all()) {
            modified = true;
        }
        auto fc = Inkscape::FontCollections::get();
        if (!fc->get_selected_collections().empty()) {
            fc->clear_selected_collections();
            modified = true;
        }
        if (modified) {
            add_categories();
            update_filterbar();
        }
    });

    auto search = &get_widget<Gtk::SearchEntry2>(_builder, "font-search");
    search->signal_changed().connect([this](){ filter(); });

    auto set_row_height = [font_renderer, this](int font_size_percent) {
        font_renderer->_font_size = font_size_percent;
        // TODO: use pango layout to calc sizes
        int hh = (font_renderer->_show_font_name ? 10 : 0) + 18 * font_renderer->_font_size / 100;
        font_renderer->set_fixed_size(-1, hh);
        // resize rows
        _font_list.set_fixed_height_mode(false);
        _font_list.set_fixed_height_mode();
    };
    auto set_grid_size = [=](int font_size_percent) {
        grid_renderer->_font_size = font_size_percent;
        set_grid_cell_size(grid_renderer, font_size_percent);
    };

    auto prefs = Inkscape::Preferences::get();

    font_renderer->_font_size = prefs->getIntLimited(_prefs + "/preview-size", 200, 100, 800);
    auto size = &get_widget<Gtk::Scale>(_builder, "preview-font-size");
    size->set_format_value_func([](double val){
        return Glib::ustring::format(std::fixed, std::setprecision(0), val) + "%";
    });
    size->set_value(font_renderer->_font_size);
    size->signal_value_changed().connect([=, this](){
        auto font_size = size->get_value();
        set_row_height(font_size);
        set_grid_size(font_size);
        prefs->setInt(_prefs + "/preview-size", font_size);
        // resize
        filter();
    });

    auto show_names = &get_widget<Gtk::CheckButton>(_builder, "show-font-name");
    auto set_show_names = [=, this](bool show) {
        font_renderer->_show_font_name = show;
        prefs->setBool(_prefs + "/show-font-names", show);
        set_row_height(font_renderer->_font_size);
        _font_list.set_grid_lines(show ? Gtk::TreeView::GridLines::HORIZONTAL : Gtk::TreeView::GridLines::NONE);
        // resize
        filter();
    };
    auto show = prefs->getBool(_prefs + "/show-font-names", true);
    set_show_names(show);
    show_names->set_active(show);
    show_names->signal_toggled().connect([=](){
        bool show = show_names->get_active();
        set_show_names(show);
    });

    // sample text to show for each font; empty to show font name
    auto sample = &get_widget<Gtk::Entry>(_builder, "sample-text");
    auto sample_text = prefs->getString(_prefs + "/sample-text");
    sample->set_text(sample_text);
    font_renderer->_sample_text = sample_text;
    sample->signal_changed().connect([=, this](){
        auto text = sample->get_text();
        font_renderer->_sample_text = text;
        prefs->setString(_prefs + "/sample-text", text);
        _font_list.queue_draw();
    });
    // sample text for grid
    auto grid_sample = &get_widget<Gtk::Entry>(_builder, "grid-sample");
    auto sample_grid_text = prefs->getString(_prefs + "/grid-text", "Aa");
    grid_sample->set_text(sample_grid_text);
    grid_renderer->_sample_text = sample_grid_text;
    grid_sample->signal_changed().connect([=, this](){
        auto text = grid_sample->get_text();
        grid_renderer->_sample_text = text.empty() ? "?" : text;
        prefs->setString(_prefs + "/grid-text", text);
        _font_grid.queue_draw();
    });

    // Populate samples submenu from stringlist
    auto samples_submenu = get_object<Gio::Menu>(_builder, "samples-submenu");
    auto samples_stringlist = get_object<Gtk::StringList>(_builder, "samples-stringlist");

    auto truncate = [] (Glib::ustring const &text) {
        constexpr int N = 30; // limit number of characters in label
        if (text.length() <= N) {
            return text;
        }

        auto substr = text.substr(0, N);
        auto pos = substr.rfind(' ');
        // do we have a space somewhere close to the limit of N chars?
        if (pos != Glib::ustring::npos && pos > N - N / 4) {
            // if so, truncate at space
            substr = substr.substr(0, pos);
        }
        substr += "\u2026"; // add ellipsis to truncated content
        return substr;
    };

    for (int i = 0, n_items = samples_stringlist->get_n_items(); i < n_items; i++) {
        auto text = samples_stringlist->get_string(i);
        auto menu_item = Gio::MenuItem::create(truncate(text), "");
        menu_item->set_action_and_target("win.set-sample", Glib::Variant<Glib::ustring>::create(text));
        samples_submenu->append_item(menu_item);
    }

    // Hook up action used by samples submenu
    auto action_group = Gio::SimpleActionGroup::create();
    action_group->add_action_with_parameter("set-sample", Glib::Variant<Glib::ustring>::variant_type(), [=] (Glib::VariantBase const &param) {
        auto param_str = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(param).get();
        sample->set_text(param_str);
    });
    insert_action_group("win", action_group);

    _text_column.set_sizing(Gtk::TreeViewColumn::Sizing::FIXED);
    _text_column.pack_start(*_cell_icon_renderer, false);
    _text_column.add_attribute(ico->property_icon_name(), g_column_model.icon_name);
    _cell_renderer->property_ellipsize() = Pango::EllipsizeMode::END;
    _text_column.pack_start(*_cell_renderer, true);
    _text_column.set_fixed_width(100); // limit minimal width to keep entire dialog narrow; column can still grow
    _text_column.set_cell_data_func(*_cell_renderer, [=](Gtk::CellRenderer* r, const Gtk::TreeModel::const_iterator& it) {
        Gtk::TreeModel::ConstRow row = *it;
        get_cell_data_func(r, row);
    });
    _text_column.set_expand();
    _font_list.append_column(_text_column);

    _font_list.set_fixed_height_mode();
    set_row_height(font_renderer->_font_size);
    //todo: restore grid size and view?
    _font_list.set_search_column(-1); // disable, we have a separate search/filter
    _font_list.set_enable_search(false);
    _font_list.set_model(_font_list_store);

    _font_grid.pack_start(*_grid_renderer);
    grid_renderer->set_fixed_height_from_font(-1);
    set_grid_size(grid_renderer->_font_size);
    // grid_renderer->_sample_text = "Aa";
    _font_grid.set_cell_data_func(*grid_renderer, [=](const Gtk::TreeModel::const_iterator& it) {
        Gtk::TreeModel::ConstRow row = *it;
        get_cell_data_func(grid_renderer, row);
    });
    // show font name in a grid tooltip
    _font_grid.signal_query_tooltip().connect([this](int x, int y, bool kbd, const Glib::RefPtr<Gtk::Tooltip>& tt){
        Gtk::TreeModel::iterator iter;
        Glib::ustring name;
        if (_font_grid.get_tooltip_context_iter(x, y, kbd, iter)) {
            const auto& font = iter->get_value(g_column_model.font);
            name = get_font_name(iter);
            tt->set_text(name);
        }
        return !name.empty();
    }, true);
    _font_grid.property_has_tooltip() = true;

    auto font_selected = [this](const FontInfo& font) {
        if (_update.pending()) return;

        auto scoped = _update.block();
        auto vars = font.variations;
        if (vars.empty() && font.variable_font) {
            vars = Inkscape::get_inkscape_fontspec(font.ff, font.face, font.variations);
        }
        _font_variations.update(vars);
        _signal_changed.emit();
    };

    _selection_changed = _font_grid.signal_selection_changed().connect([font_selected, this](){
        auto sel = _font_grid.get_selected_items();
        if (sel.size() == 1) {
            auto it = _font_list_store->get_iter(sel.front());
            const Inkscape::FontInfo& font = (*it)[g_column_model.font];
            font_selected(font);
        }
    });

    auto show_grid = &get_widget<Gtk::ToggleButton>(_builder, "view-grid");
    auto show_list = &get_widget<Gtk::ToggleButton>(_builder, "view-list");
    auto set_list_view_mode = [prefs, this](bool show_list) {
        auto& list = get_widget<Gtk::ScrolledWindow>(_builder, "list");
        auto& grid = get_widget<Gtk::ScrolledWindow>(_builder, "grid");
        // TODO: sync selection between font widgets
        if (show_list) {
            grid.set_visible(false);
            _font_grid.unset_model();
            list.set_visible();
        }
        else {
            list.set_visible(false);
            _font_grid.set_model(_font_list_store);
            grid.set_visible();
        }
        _view_mode_list = show_list;
        prefs->setBool(_prefs + "/list-view-mode", show_list);
    };
    auto list_mode = prefs->getBool(_prefs + "/list-view-mode", true);
    if (list_mode) show_list->set_active(); else show_grid->set_active();
    set_list_view_mode(list_mode);
    show_list->signal_toggled().connect([=]() { set_list_view_mode(true); });
    show_grid->signal_toggled().connect([=]() { set_list_view_mode(false); });

    _fonts.clear();
    _initializing = 0;
    _info_box.set_visible(false);
    _progress_box.show();

    auto prepare_tags = [this](){
        // prepare dynamic tags
        for (auto&& f : _fonts) {
            auto kind = f.family_kind >> 8;
            if (kind == 10) {
                _font_tags.tag_font(f.face, "script");
            }
            else if (kind >= 1 && kind <= 5) {
                _font_tags.tag_font(f.face, "serif");
            }
            else if (kind == 8) {
                _font_tags.tag_font(f.face, "sans");
            }
            else if (kind == 12) {
                _font_tags.tag_font(f.face, "symbols");
            }

            if (f.monospaced) {
                _font_tags.tag_font(f.face, "monospace");
            }
            if (f.variable_font) {
                _font_tags.tag_font(f.face, "variable");
            }
            if (f.oblique) {
                _font_tags.tag_font(f.face, "oblique");
            }
        }
    };

    _font_stream = FontDiscovery::get().connect_to_fonts([=, this](const FontDiscovery::MessageType& msg){
        if (auto r = Async::Msg::get_result(msg)) {
            _fonts = **r;
            sort_fonts(_order);
            prepare_tags();
            filter();
        }
        else if (auto p = Async::Msg::get_progress(msg)) {
            // show progress
            _info_box.set_visible(false);
            _progress_box.set_visible();
            auto& progress = get_widget<Gtk::ProgressBar>(_builder, "init-progress");
            progress.set_fraction(std::get<double>(*p));
            progress.set_text(std::get<Glib::ustring>(*p));
            auto&& family = std::get<std::vector<FontInfo>>(*p);
            _fonts.insert(_fonts.end(), family.begin(), family.end());
            auto delta = _fonts.size() - _initializing;
            // refresh fonts; at first more frequently, every new 100, but then more slowly, as it gets costly
            if (delta > 500 || (_fonts.size() < 500 && delta > 100)) {
                _initializing = _fonts.size();
                sort_fonts(_order);
                filter();
            }
        }
        else if (Async::Msg::is_finished(msg)) {
            // hide progress
            _progress_box.set_visible(false);
            _info_box.set_visible();
        }
    });

    _font_size_scale.get_adjustment()->set_lower(0);
    _font_size_scale.get_adjustment()->set_upper(g_font_sizes.size() - 1);
    _font_size_scale.signal_value_changed().connect([this](){
        if (_update.pending()) return;

        auto scoped = _update.block();
        auto size = index_to_font_size(_font_size_scale.get_value());
        _font_size.get_entry()->set_text(std::to_string(size));
        _signal_changed.emit();
    });

    _font_size.signal_changed().connect([this](){
        if (_update.pending()) return;

        auto scoped = _update.block();
        auto text = _font_size.get_active_text();
        if (!text.empty()) {
            auto size = ::atof(text.c_str());
            if (size > 0) {
                _font_size_scale.set_value(font_size_to_index(size));
                _signal_changed.emit();
            }
        }
    });
    _font_size.set_active_text("10");
    _font_size.get_entry()->set_max_width_chars(6);

    sort_fonts(Inkscape::FontOrder::by_name);

    _font_list.get_selection()->signal_changed().connect([=, this](){
        if (auto iter = _font_list.get_selection()->get_selected()) {
            const Inkscape::FontInfo& font = (*iter)[g_column_model.font];
            font_selected(font);
        }
    }, false);

    // double-click:
    _font_list.signal_row_activated().connect([this](const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn*){
        if (!_update.pending()) {
            auto scoped = _update.block();
            _signal_apply.emit();
        }
    });

    _font_tags.get_signal_tag_changed().connect([this](const FontTag* ftag, bool selected){
        sync_font_tag(ftag, selected);
    });

    auto& filter_popover = get_widget<Gtk::Popover>(_builder, "filter-popover");
    filter_popover.signal_show().connect([this](){
        // update tag checkboxes
        add_categories();
        update_filterbar();
    }, false);

    _font_collections_update = Inkscape::FontCollections::get()->connect_update([this]() {
        add_categories();
        update_filterbar();
        filter();
    });
    _font_collections_selection = Inkscape::FontCollections::get()->connect_selection_update([this](){
        add_categories();
        update_filterbar();
        filter();
    });
}

void FontList::sort_fonts(Inkscape::FontOrder order) {
    Inkscape::sort_fonts(_fonts, order, true);

    if (const char* icon = get_sort_icon(order)) {
        auto& sort = get_widget<Gtk::Image>(_builder, "sort-icon");
        sort.set_from_icon_name(icon);
    }

    filter();
}

bool FontList::select_font(const Glib::ustring& fontspec) {
    bool found = false;

    _font_list_store->foreach([&](const Gtk::TreeModel::Path& path, const Gtk::TreeModel::iterator& iter){
        const auto& row = *iter;
        const auto& font = row.get_value(g_column_model.font);
        if (!font.ff) {
            auto spec = row.get_value(g_column_model.alt_fontspec);
            if (spec == fontspec) {
                _font_list.get_selection()->select(row.get_iter());
                _font_grid.select_path(path);
                scroll_to_row(path);
                found = true;
                return true; // stop
            }
        }
        else {
            const auto& font = row.get_value(g_column_model.font);
            auto spec = Inkscape::get_inkscape_fontspec(font.ff, font.face, font.variations);
            if (spec == fontspec) {
                _font_list.get_selection()->select(row.get_iter());
                _font_grid.select_path(path);
                scroll_to_row(path);
                found = true;
                return true; // stop
            }
        }
        return false; // continue
    });

    return found;
}

void FontList::filter() {
    auto scoped = _update.block();
    // todo: save selection
    Inkscape::FontInfo selected;
    auto it = _font_list.get_selection()->get_selected();
    if (it) {
        selected = it->get_value(g_column_model.font);
    }

    auto& search = get_widget<Gtk::SearchEntry2>(_builder, "font-search");
    // Not used: extra search terms; use collections instead
    // auto& oblique = get_widget<Gtk::CheckButton>(_builder, "id-oblique");
    // auto& monospaced = get_widget<Gtk::CheckButton>(_builder, "id-monospaced");
    // auto& others = get_widget<Gtk::CheckButton>(_builder, "id-other");
    Show params;
    // Not used
    // params.monospaced = monospaced.get_active();
    // params.oblique = oblique.get_active();
    // params.others = others.get_active();
    populate_font_store(search.get_text(), params);

    if (!_current_fspec.empty()) {
        add_font(_current_fspec, false);
    }

    if (it) {
        // reselect if that font is still available
        //TODO
    }
}

Glib::ustring get_font_icon(const FontInfo& font, bool missing_font) {
    if (missing_font) {
        return "missing-element-symbolic";
    }
    else if (font.variable_font) {
        return ""; // TODO: add icon for variable fonts?
    }
    else if (font.synthetic) {
        return "generic-font-symbolic";
    }
    return Glib::ustring();
}

// add fonts to the font store taking filtring params into account
void FontList::populate_font_store(Glib::ustring text, const Show& params) {
    auto filter = text.lowercase();

    auto active_categories = _font_tags.get_selected_tags();
    auto apply_categories = !active_categories.empty();

    auto fc = Inkscape::FontCollections::get();
    auto font_collections = fc->get_selected_collections();
    auto apply_font_collections = !font_collections.empty();

    _font_list.set_visible(false); // hide tree view temporarily to speed up rebuild
    _font_grid.set_visible(false);
    _font_list_store->freeze_notify();
    _font_list_store->clear();
    _extra_fonts = 0;

    for (auto&& f : _fonts) {
        bool filter_in = false;

        if (!text.empty()) {
            auto name1 = get_full_name(f);
            if (name1.lowercase().find(filter) == Glib::ustring::npos) continue;
        }

        if (apply_categories || apply_font_collections) {
            if (apply_categories) {
                auto&& set = _font_tags.get_font_tags(f.face);
                for (auto&& ftag : active_categories) {
                    if (set.contains(ftag.tag)) {
                        filter_in = true;
                        break;
                    }
                }
            }

            if (!filter_in && apply_font_collections) {
                for (auto& col : font_collections) {
                    if (fc->is_font_in_collection(col, f.ff->get_name())) {
                        filter_in = true;
                        break;
                    }
                }
            }

            if (!filter_in) continue;
        }

        Gtk::TreeModel::iterator treeModelIter = _font_list_store->append();
        auto& row = *treeModelIter;
        row[g_column_model.font] = f;
        row[g_column_model.alt_fontspec] = Glib::ustring();
        row[g_column_model.icon_name] = get_font_icon(f, false);
    }

    _font_list_store->thaw_notify();
    _font_list.set_visible(); // restore visibility
    _font_grid.set_visible();

    update_font_count();
}

void FontList::update_font_count() {
    auto& font_count = get_widget<Gtk::Label>(_builder, "font-count");
    auto count = _font_list_store->children().size();
    auto total = _fonts.size();
    // count could be larger than total if we insert "missing" font(s)
    auto label = count >= total ? C_("N-of-fonts", "All fonts") : Glib::ustring::format(count, ' ', C_("N-of-fonts", "of"), ' ', total, ' ', C_("N-of-fonts", "fonts"));
    font_count.set_text(label);
}

double FontList::get_fontsize() const {
    auto text = _font_size.get_entry()->get_text();
    if (!text.empty()) {
        auto size = ::atof(text.c_str());
        if (size > 0) return size;
    }
    return _current_fsize;
}

Gtk::TreeModel::iterator FontList::get_selected_font() const {
    if (_view_mode_list) {
        return _font_list.get_selection()->get_selected();
    }
    else {
        auto sel = _font_grid.get_selected_items();
        if (sel.size() == 1) return _font_list_store->get_iter(sel.front());
    }
    return {};
}

Glib::ustring FontList::get_fontspec() const {
    if (auto iter = get_selected_font()) { //_font_list.get_selection()->get_selected()) {
        const auto& font = iter->get_value(g_column_model.font);
        // auto font_class = iter->get_value(g_column_model.font_class);
        if (font.ff) {
            auto variations = _font_variations.get_pango_string(true);
            auto fspec = Inkscape::get_inkscape_fontspec(font.ff, font.face, variations);
            return fspec;
        }
        else {
            // missing fonts don't have known variation that we could tweak,
            // so ignore _font_variations UI and simply return alt_fontspec
            auto&& name = iter->get_value(g_column_model.alt_fontspec);
            return name;
        }
    }
    return "sans-serif"; // no selection
}

void FontList::set_current_font(const Glib::ustring& family, const Glib::ustring& face) {
    if (_update.pending()) return;

    auto scoped = _update.block();

    auto fontspec = Inkscape::get_fontspec(family, face);
    if (fontspec == _current_fspec) {
        auto fspec = get_fontspec_without_variants(fontspec);
        select_font(fspec);
        return;
    }
    _current_fspec = fontspec;

    if (!fontspec.empty()) {
        _font_variations.update(fontspec);
        add_font(fontspec, true);
    }
}

void FontList::set_current_size(double size) {
    _current_fsize = size;
    if (_update.pending()) return;

    auto scoped = _update.block();
    CSSOStringStream os;
    os.precision(3);
    os << size;
    _font_size_scale.set_value(font_size_to_index(size));
    _font_size.get_entry()->set_text(os.str());
}

void FontList::add_font(const Glib::ustring& fontspec, bool select) {
    bool found = select_font(fontspec); // found in the tree view?
    if (found) return;

    auto it = std::find_if(begin(_fonts), end(_fonts), [&](const FontInfo& f){
        return Inkscape::get_inkscape_fontspec(f.ff, f.face, f.variations) == fontspec;
    });

    // fonts with variations will not be found, we need to remove " @ axis=value" part
    auto fspec = get_fontspec_without_variants(fontspec);
    // auto at = fontspec.rfind('@');
    if (it == end(_fonts) && fspec != fontspec) {
        // try to match existing font
        it = std::find_if(begin(_fonts), end(_fonts), [&](const FontInfo& f){
            return Inkscape::get_inkscape_fontspec(f.ff, f.face, f.variations) == fspec;
        });
        if (it != end(_fonts)) {
            bool found = select_font(fspec); // found in the tree view?
            if (found) return;
        }
    }

    if (it != end(_fonts)) {
        // font found in the "all fonts" vector, but
        // this font is filtered out; add it temporarily to the tree list
        Gtk::TreeModel::iterator iter = _font_list_store->prepend();
        auto& row = *iter;
        row[g_column_model.font] = *it;
        row[g_column_model.injected] = true;
        row[g_column_model.alt_fontspec] = Glib::ustring();
        row[g_column_model.icon_name] = get_font_icon(*it, false);

        if (select) {
            _font_list.get_selection()->select(row.get_iter());
            auto path = _font_list_store->get_path(iter);
            scroll_to_row(path);
        }

        ++_extra_fonts; // extra font entry inserted
    }
    else {
        bool missing_font = true;
        Inkscape::FontInfo subst;

        auto desc = Pango::FontDescription(fontspec);
        auto vars = desc.get_variations();
        if (!vars.empty()) {
            // font with variantions; check if we have matching family
            subst.variations = vars;

            auto family = desc.get_family();
            it = std::find_if(begin(_fonts), end(_fonts), [&](const FontInfo& f){
                return f.ff->get_name() == family;
            });
            if (it != end(_fonts)) {
                missing_font = false;
                subst.ff = it->ff;
            }
        }

        auto&& children = _font_list_store->children();
        Gtk::TreeModel::iterator iter;
        if (!children.empty() && children[0][g_column_model.injected]) {
            // reuse "injected font" entry
            iter = children[0].get_iter();
        }
        else {
            // font not found; insert a placeholder to show injected font: font is used in a document,
            // but either not available in the system (it's missing) or it is a variant of the variable font
            iter = _font_list_store->prepend();
        }
        auto& row = *iter;
        row[g_column_model.font] = subst;
        row[g_column_model.injected] = true;
        row[g_column_model.alt_fontspec] = fontspec;// TODO fname?
        row[g_column_model.icon_name] = get_font_icon(subst, missing_font);

        if (select) {
            _font_list.get_selection()->select(iter);
            auto path = _font_list_store->get_path(iter);
            scroll_to_row(path);
        }

        ++_extra_fonts; // extra font entry for a missing font added
    }
    update_font_count();
}

Gtk::Box* FontList::create_pill_box(const Glib::ustring& display_name, const Glib::ustring& tag, bool tags) {
    auto box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL);
    auto text = Gtk::make_managed<Gtk::Label>(display_name);
    text->set_ellipsize(Pango::EllipsizeMode::END);
    text->set_max_width_chars(10);
    text->set_tooltip_text(display_name);
    auto close = Gtk::make_managed<Gtk::Button>();
    close->set_has_frame(false);
    close->set_image_from_icon_name("close-button-symbolic");
    close->set_valign(Gtk::Align::CENTER);
    if (tags) {
        close->signal_clicked().connect([=, this](){
            // remove category from current filter
            update_categories(tag, false);
        });
    }
    else {
        close->signal_clicked().connect([=](){
            // remove collection from current filter
            Inkscape::FontCollections::get()->update_selected_collections(tag);
        });
    }
    box->get_style_context()->add_class("tag-box");
    box->append(*text);
    box->append(*close);
    box->set_valign(Gtk::Align::CENTER);
    return box;
}

// show selected font categories in the filter bar
void FontList::update_filterbar() {
    // brute force approach at first
    for (auto&& btn : _tag_box.get_children()) {
        _tag_box.remove(*btn);
    }

    for (auto&& ftag : _font_tags.get_selected_tags()) {
        auto pill = create_pill_box(ftag.display_name, ftag.tag, true);
        _tag_box.append(*pill);
    }

    for (auto&& collection : Inkscape::FontCollections::get()->get_selected_collections()) {
        auto pill = create_pill_box(collection, collection, false);
        _tag_box.append(*pill);
    }
}

void FontList::update_categories(const std::string& tag, bool select) {
    if (_update.pending()) return;

    auto scoped = _update.block();

    if (!_font_tags.select_tag(tag, select)) return;

    // update UI
    update_filterbar();

    // apply filter
    filter();
}

void FontList::add_categories() {
    for (auto row : _tag_list.get_children()) {
        if (row) _tag_list.remove(*row);
    }

    auto add_row = [this](Gtk::Widget* w){
        auto row = Gtk::make_managed<Gtk::ListBoxRow>();
        row->set_can_focus(false);
        row->set_child(*w);
        row->set_sensitive(w->get_sensitive());
        _tag_list.append(*row);
    };

    for (auto& tag : _font_tags.get_tags()) {
        auto btn = Gtk::make_managed<Gtk::CheckButton>("");
        // automatic collections in italic
        auto& label = *Gtk::make_managed<Gtk::Label>();
        label.set_markup("<i>" + tag.display_name + "</i>");
        btn->set_child(label);
        btn->set_active(_font_tags.is_tag_selected(tag.tag));
        btn->signal_toggled().connect([=, this](){
            // toggle font category
            update_categories(tag.tag, btn->get_active());
        });
        add_row(btn);
    }

    // insert user collections
    auto fc = Inkscape::FontCollections::get();
    auto font_collections = fc->get_collections();
    if (!font_collections.empty()) {
        auto sep = Gtk::make_managed<Gtk::Separator>();
        sep->set_margin_bottom(3);
        sep->set_margin_top(3);
        sep->set_sensitive(false);
        add_row(sep);
    }
    for (auto& col : font_collections) {
        auto btn = Gtk::make_managed<Gtk::CheckButton>(col);
        btn->set_active(fc->is_collection_selected(col));
        btn->signal_toggled().connect([=](){
            // toggle font system collection
            fc->update_selected_collections(col);
        });
        add_row(btn);
    }
}

void FontList::sync_font_tag(const FontTag* ftag, bool selected) {
    if (!ftag) {
        // many/all tags changed
        add_categories();
        update_filterbar();
    }
    //todo as needed
}

void FontList::scroll_to_row(Gtk::TreePath path) {
    if (_view_mode_list) {
        // delay scroll request to let widget layout complete (due to hiding or showing variable font widgets);
        // keep track of connection so we can disconnect in a destructor if it is still pending at that point
        _scroll = Glib::signal_timeout().connect([=, this](){
            _font_list.scroll_to_row(path);
            return false; // <- false means disconnect
        }, 50, Glib::PRIORITY_LOW);
        // fudge factor of 50ms; ideally wait for layout pass to complete before scrolling to the row
    }
    else {
        // scroll grid
        //todo
    }
}

} // namespaces
