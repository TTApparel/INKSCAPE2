// SPDX-License-Identifier: GPL-2.0-or-later
//
// Created by Michael Kowalski on 1/31/25.
//

#include "settings-dialog.h"

#include <array>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <memory>
#include <numeric>
#include <optional>
#include <regex>
#include <string>
#include <string_view>
#include <vector>

#include <gdk/gdkkeysyms.h>
#include <gdkmm/display.h>
#include <gdkmm/enums.h>
#include <glibmm/fileutils.h>
#include <glibmm/i18n.h>
#include <glibmm/markup.h>
#include <glibmm/ustring.h>
#include <gtkmm/accelerator.h>
#include <gtkmm/accelkey.h>
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/dropdown.h>
#include <gtkmm/entry.h>
#include <gtkmm/enums.h>
#include <gtkmm/eventcontrollerfocus.h>
#include <gtkmm/eventcontrollerkey.h>
#include <gtkmm/fontchooserdialog.h>
#include <gtkmm/label.h>
#include <gtkmm/object.h>
#include <gtkmm/overlay.h>
#include <gtkmm/popover.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/settings.h>
#include <gtkmm/shortcuttrigger.h>
#include <gtkmm/sizegroup.h>
#include <gtkmm/switch.h>
#include <gtkmm/textview.h>
#include <gtkmm/togglebutton.h>
#include <gtkmm/widget.h>
#include <sigc++/signal.h>

#include "inkscape-application.h"
#include "io/resource.h"
#include "ui/builder-utils.h"
#include "ui/containerize.h"
#include "ui/modifiers.h"
#include "ui/popup-menu.h"
#include "ui/shortcuts.h"
#include "ui/util.h"
#include "ui/widget/color-picker.h"
#include "ui/widget/generic/spin-button.h"
#include "ui/widget/preference-unit-menu.h"
#include "ui/widget/preference-widgets.h"
#include "ui/widget/preferences-widget.h"
#include "ui/widget/unit-menu.h"
#include "preferences.h"
#include "settings-helpers.h"
#include "util/action-accel.h"
#include "util-string/ustring-format.h"
#include "util/key-helpers.h"
#include "util/units.h"
#include "xml/attribute-record.h"
#include "xml/node.h"
#include "xml/repr.h"

namespace Inkscape::UI::Dialog {

static Glib::ustring join(const std::vector<Glib::ustring>& accels) {
    auto capacity = std::accumulate(accels.begin(), accels.end(), std::size_t{0},
        [](std::size_t capacity, auto& accel){ return capacity + accel.size() + 2; });
    Glib::ustring result;
    result.reserve(capacity);
    for (auto& accel : accels) {
        if (!result.empty()) result += ", ";
        result += accel;
    }
    return result;
}

struct PreferencesIO : ReadWrite::IO {
    PreferencesIO(Glib::RefPtr<Gdk::Display> display): _display(display) {}
    ~PreferencesIO() override = default;

    std::optional<std::string> read(const std::string& path) override {
        // printf("get at '%s'\n", path.c_str());
        if (path.starts_with("/shortcuts/")) {
            // retrieve shortcut
            auto subpath = path.substr(strlen("/shortcuts/"));
            if (subpath.starts_with("modifiers/")) {
                // modifier keys are handled separately
                subpath = subpath.substr(strlen("modifiers/"));
                auto sep = subpath.find('/');
                auto m = Modifiers::Modifier::get(subpath.substr(0, sep).c_str());
                if (m) {
                    auto mask = m->get_and_mask();
                    if (mask <= 0) {
                        return "0";
                    }
                    auto key = subpath.substr(sep + 1);
                    if (key == "shift") {
                        return mask & Modifiers::Key::SHIFT ? "1" : "0";
                    }
                    else if (key == "ctrl") {
                        return mask & Modifiers::Key::CTRL ? "1" : "0";
                    }
                    else if (key == "alt") {
                        return mask & Modifiers::Key::ALT ? "1" : "0";
                    }
                    else if (key == "meta") {
                        return mask & Modifiers::Key::META ? "1" : "0";
                    }
                }
                return {};
            }
            else {
                Util::ActionAccel accel(subpath);
                return Util::format_accel_keys(_display, accel.getKeys());
            }
        }
        else if (auto entry = Preferences::get()->getEntry(path); entry.isSet()) {
            return {entry.getString()};
        }
        return {};
    }

    void write(const std::string& path, std::string_view value) override {
        // printf("set '%s' at '%s'\n", value.data(), path.c_str());
        if (value.empty()) {
            printf("  Empty pref value!\n");
        }
        else if (path.starts_with("/shortcuts/")) {
            //todo: set shortcuts
        }
        else {
            Preferences::get()->setString(path, Glib::ustring(value.data(), value.size()));
        }
    }

    bool is_valid(const std::string& path) override {
        return read(path).has_value();

        //todo: for later...
        return Preferences::get()->is_default_valid(path);
    }

    Glib::RefPtr<Gdk::Display> _display;
};

namespace {

// Size of a single column in a 12-column grid; settings are built on such a grid
constexpr int ONE_COLUMN = 24;
constexpr int WHOLE  = 12 * ONE_COLUMN;
constexpr int HALF   =  6 * ONE_COLUMN;
constexpr int THIRD  =  4 * ONE_COLUMN;
constexpr int QUARTER=  3 * ONE_COLUMN;
constexpr int SIXTH  =  2 * ONE_COLUMN;

const Glib::Quark NODE_KEY{"node-element"};

XML::Node* get_widget_node(const Gtk::Widget* widget) {
    if (!widget) return nullptr;

    return static_cast<XML::Node*>(const_cast<Gtk::Widget*>(widget)->get_data(NODE_KEY));
}

const Glib::ustring settings_path("/dialogs/settings/");

const char* get_modifier_key_name(GdkModifierType key) {
    static std::map<GdkModifierType, std::string, std::less<>> key_names = {
#ifdef __APPLE__
        { GDK_SHIFT_MASK, _("⇧") },
        { GDK_CONTROL_MASK,  _("^") },
        { GDK_ALT_MASK,   _("⌥") },
        { GDK_META_MASK,  _("⌘") },
#else
        { GDK_SHIFT_MASK, _("Shift") },
        { GDK_CONTROL_MASK,  _("Ctrl") },
        { GDK_ALT_MASK,   _("Alt") },
        { GDK_META_MASK,  _("Meta") },
#endif
        { GDK_SUPER_MASK, _("Super") },
        { GDK_HYPER_MASK, _("Hyper") },
    };

    if (auto it = key_names.find(key); it != key_names.end()) {
        return it->second.c_str();
    }
    return "";
}

Glib::ustring get_modifier_name(std::string_view str) {
    if (str.empty()) return {};

    static std::map<std::string, std::string, std::less<>> key_names = {
        { "Shift", _("Shift") },
#ifdef __APPLE__
        { "Ctrl",  _("Control") },
        { "Alt",   _("Option") },
        { "Meta",  _("Command") },
#else
        { "Ctrl",  _("Ctrl") },
        { "Alt",   _("Alt") },
        { "Meta",  _("Meta") },
#endif
        { "Super", _("Super") },
        { "Hyper", _("Hyper") },
    };

    if (auto it = key_names.find(str); it != key_names.end()) {
        return std::string(it->second);
    }

    return Glib::ustring(str.data(), str.length());
}

XML::Document* get_ui_xml() {
    auto fname = IO::Resource::get_path_string(IO::Resource::SYSTEM, IO::Resource::UIS, "settings-dialog.xml");
    auto content = Glib::file_get_contents(fname);
    return sp_repr_read_mem(content.data(), content.length(), nullptr);
}

std::string_view element_name(const XML::Node* node) {
    auto name = node->name();
    // ReSharper disable once CppDFALocalValueEscapesFunction
    return std::string_view(name, name ? strlen(name) : 0);
}

std::string_view element_attr(const XML::Node* node, const char* attr_name) {
    if (!node) return {};
    auto attrib = node->attribute(attr_name);
    // ReSharper disable once CppDFALocalValueEscapesFunction
    return std::string_view(attrib, attrib ? strlen(attrib) : 0);
}

Glib::ustring to_string(std::string_view str) {
    return Glib::ustring(str.data(), str.length());
}

double to_number(std::string_view str, double default_val = 0.0) {
    if (str.empty()) return default_val;

    // return std::from_chars(str.data(), str.data() + str.length(), d);

    // this is cheating, b/c those attribute strings are null-terminated
    return strtod(str.data(), nullptr);
}

std::string get_widget_id(const Gtk::Widget* widget) {
    if (auto node = get_widget_node(widget)) {
        return to_string(element_attr(node, "id"));
    }
    return {};
}

int to_size(std::string_view size, int default_size) {
    if (size.empty()) return default_size;

    if (size == "whole") {
        return WHOLE;
    }
    else if (size == "half") {
        return HALF;
    }
    else if (size == "third") {
        return THIRD;
    }
    else if (size == "quarter") {
        return QUARTER;
    }
    else if (size == "sixth") {
        return SIXTH;
    }
    else {
        std::cerr << "Element size request " << size << " not recognized";
        return default_size;
    }
}

std::string to_path(XML::Node* node) {
    std::string abs_path;
    while (node) {
        auto path_segment = element_attr(node, "path");
        if (path_segment.size()) {
            // prepend path segment
            if (!abs_path.empty()) abs_path.insert(0, "/");
            abs_path.insert(abs_path.begin(), path_segment.data(), path_segment.data() + path_segment.length());
            if (abs_path[0] == '/') break;
        }

        node = node->parent();
    }
    return abs_path;
}

bool read_bool(ReadWrite::IO* io, const std::string& path) {
    auto value = io->read(path);
    if (value->empty()) {
        // std::cerr << "Missing preference value for '" << path << "'. Fix preferences-skeleton.h file" << std::endl;
        return false;
    }

    static std::regex on{"true|on|1"};
    std::smatch match;
    if (std::regex_match(*value, match, on)) {
        return !match.empty();
    }
    return false;
}
bool read_bool(XML::Node* node, ReadWrite::IO* io) {
    return read_bool(io, to_path(node));
}

void validate_path(Gtk::Widget* widget, ReadWrite::IO* io, const std::string& path) {
    if (io->is_valid(path)) {
        widget->remove_css_class("error");
    }
    else {
        widget->add_css_class("error");
    }
}

void validate(Gtk::Widget* widget, XML::Node* node, ReadWrite::IO* io) {
    if (element_attr(node->parent(), "validation") == "off") {
        return;
    }
    auto check_node = node;
    // verify path requirement - for all radio buttons it is on a parent node
    if (element_name(node) == "radiobutton" || element_attr(node->parent(), "type") == "radio") {
        check_node = node->parent();
    }
    // detect missing path attribute
    auto path_segment = element_attr(check_node, "path");
    if (path_segment.empty()) {
        auto name = element_name(check_node);
        std::cerr << "Settings - element '" << (name.empty() ? "?" : name) << "' without 'path' property detected\n";
    }
    validate_path(widget, io, to_path(node));
}

// initialize widget with a value read from settings
void set_widget(auto* button, XML::Node* node, ReadWrite::IO* io) {
    auto active_value = element_attr(node, "value");
    if (active_value.empty()) {
        auto value = read_bool(node, io);
        button->set_active(value);
    }
    else {
        auto value = io->read(to_path(node)).value_or("");
        button->set_active(value == active_value);
    }
}
// ditto, but for spin button
void set_widget(Widget::InkSpinButton* button, XML::Node* node, ReadWrite::IO* io) {
    auto value = io->read(to_path(node)).value_or("0");
    button->set_value(std::stod(value));
}

void set_widget(Gtk::Switch* switch_widget, XML::Node* node, ReadWrite::IO* io) {
    auto value = read_bool(io, to_path(node));
    switch_widget->set_active(value);
}

void set_widget(Gtk::TextView* text, XML::Node* node, ReadWrite::IO* io, char separator) {
    auto value = io->read(to_path(node)).value_or("");
    std::replace(value.begin(), value.end(), separator, '\n');
    text->get_buffer()->set_text(value);
}

Glib::ustring to_label(XML::Node* node) {
    // auto label = to_string(element_attr(node, "label"));
    auto label = element_attr(node, "label");
    auto translate = to_string(element_attr(node, "translate"));
    if (label.empty()) return {};

    if (translate != "no") {
        //NOTE: node attribute strings are null-terminatted, and gettext relies on it here
        label = gettext(label.data());
    }
    else if (label[0] == '@') { // special sequence?
        // todo - add other sequences to transform as/if needed
        return get_modifier_name(label.substr(1));
    }

    return  to_string(label);
}

Gtk::Image* create_icon(std::string_view name) {
    if (name.empty()) return nullptr;

    auto icon = Gtk::make_managed<Gtk::Image>();
    icon->add_css_class("icon");
    icon->set_from_icon_name(to_string(name));
    // this call fires warnings:
    // icon->set_icon_size(Gtk::IconSize::NORMAL);
    return icon;
}

int parse_element(XML::Node* node) {
    auto name = element_name(node);
    if (name == "gap") {
        return 8;
    }
    else if (name == "comment") {
        // skip comments
        return 0;
    }
    else {
        throw std::runtime_error(std::string("Unrecognized element in settings UI: ") + std::string(name));
    }
}

void _subst_argument(XML::Node* node, const std::string& placeholder, const std::string& arg) {
    // substitute text in attributes
    for (auto& attr : node->attributeList()) {
        if (!attr.value) continue;

        std::string_view value{attr.value, strlen(attr.value)};
        if (auto it = value.find(placeholder); it != std::string_view::npos) {
            auto val = std::string(value.substr(0, it)) + arg + std::string(value.substr(it + placeholder.length()));
            node->setAttribute(g_quark_to_string(attr.key), val);
        }
    }

    // substitute text in content
    auto content_str = node->content();
    if (content_str && *content_str) {
        auto content = std::string_view(content_str);
        if (auto it = content.find(placeholder); it != std::string_view::npos) {
            auto val = std::string(content.substr(0, it)) + arg + std::string(content.substr(it + placeholder.length()));
            node->setContent(val.c_str());
        }
    }

    // substitute in children
    for (auto el = node->firstChild(); el; el = el->next()) {
        _subst_argument(el, placeholder, arg);
    }
}

std::string_view get_attribute_name(int key) {
    auto str = g_quark_to_string(key);
    return std::string_view{str, str ? strlen(str) : 0};
}

void subst_arguments(XML::Node* source, XML::Node* dest) {
    for (auto& attr : source->attributeList()) {
        if (!attr.value) continue;
        auto name = get_attribute_name(attr.key);
        if (name == "template") continue;

        std::string placeholder;
        placeholder.reserve(1 + name.size() + 1);
        placeholder += '{';
        placeholder += name;
        placeholder += '}';

        _subst_argument(dest, placeholder, std::string(attr.value));
    }
}

// find <shortcut> element in node's children, and if there is one, return its path
std::string find_shortcut(XML::Node* node) {
    for (auto element = node->firstChild(); element; element = element->next()) {
        auto name = element_name(element);
        if (name == "shortcut") {
            return to_path(element);
        }
        else {
            auto path = find_shortcut(element);
            if (!path.empty()) return path;
        }
    }
    return {};
}

// read shortcut and set it on the label widget
void set_shortcut(ReadWrite::IO* io, std::string path, Gtk::Label* label) {
    if (!label) return;

    if (path.empty()) {
        label->set_text({});
    }
    else {
        auto keys = io->read(path);
        label->set_text(keys.has_value() ? *keys : "");
    }
}

// set group on radio-buttons to link them
void connect_radio_buttons(Gtk::Widget* parent) {
    // link checkbox radio buttons
    Gtk::CheckButton* group = nullptr;
    for (auto w : parent->get_children()) {
        if (auto radio = dynamic_cast<Gtk::CheckButton*>(w); radio && radio->has_css_class("radio-button")) {
            if (group) {
                radio->set_group(*group);
            }
            else {
                group = radio;
            }
        }
    }

    // link toggle radio buttons
    if (parent->has_css_class("radio") && parent->has_css_class("group")) {
        Gtk::ToggleButton* group = nullptr;
        for (auto w : parent->get_children()) {
            if (auto radio = dynamic_cast<Gtk::ToggleButton*>(w)) {
                if (group) {
                    radio->set_group(*group);
                }
                else {
                    group = radio;
                }
            }
        }
    }
}

using Templates = std::map<std::string, XML::Node*, std::less<>>;
using Observers = std::map<std::string, std::unique_ptr<Preferences::PreferencesObserver>>;
using Visibility = std::map<std::string, std::vector<Gtk::Widget*>>;

// Widget construction context used while traversing XML UI file
struct Context {
    Context(XML::Document& ui, const Templates& templates, ReadWrite::IO* io, Observers& observers, Visibility& visibility, int scaling_factor)
        : ui(ui), templates(templates), io(io), observers(observers), visibility(visibility), scaling_factor(scaling_factor)
    {}

    XML::Document& ui;
    const Templates& templates;
    ReadWrite::IO* io;
    Glib::RefPtr<Gtk::SizeGroup> first_col;
    Observers& observers;
    Visibility& visibility;
    int scaling_factor;
};

void add_visibility_observer(Context& ctx, Gtk::Widget* widget, XML::Node* node) {
    auto visible = element_attr(node, "visible");
    auto path = to_path(node);
    if (visible.size()) {
        path += '/';
        path += visible;
    }
    // check if widget should be hidden initially
    if (ctx.io->read(path) != element_attr(node, "value")) {
        widget->set_visible(false);
    }
    if (auto it = ctx.observers.find(path); it == ctx.observers.end()) {
        auto vis = &ctx.visibility;
        ctx.observers[path] = Preferences::PreferencesObserver::create(path, [path, vis](auto& value) {
            for (auto w : (*vis)[path]) {
                auto element = get_widget_node(w);
                if (!element) continue;
                auto on = element_attr(element, "value");
                w->set_visible(value.getString().raw() == on);
            }
        });
    }

    if (auto it = ctx.visibility.find(path); it == ctx.visibility.end()) {
        ctx.visibility[path] = {};
    }

    ctx.visibility[path].push_back(widget);
}

Gtk::Box* create_shortcut_label(const Glib::RefPtr<Gdk::Display>& display, Gtk::Box* container, int keyval, Gdk::ModifierType modifier) {
    auto create_key = [](const char* text) {
        auto key = Gtk::make_managed<Gtk::Label>(text);
        key->add_css_class("key-pillbox");
        return key;
    };

    auto accel = Util::transform_key_value(display, keyval, modifier);
    keyval = accel.get_key();
    modifier = accel.get_mod();
    int mod = static_cast<int>(modifier); 
    std::array<GdkModifierType, 6> mod_keys{GDK_SHIFT_MASK, GDK_CONTROL_MASK, GDK_ALT_MASK, GDK_META_MASK, GDK_SUPER_MASK, GDK_HYPER_MASK};
    for (auto mask : mod_keys) {
        if (mod & mask) {
            container->append(*create_key(get_modifier_key_name(mask)));
        }
    }
    auto key = Gtk::Accelerator::get_label(keyval, Gdk::ModifierType::NO_MODIFIER_MASK);

    if (key.size() == 1 && key[0] >= 'a' && key[0] <= 'z') {
        key = key.uppercase();
    }
    else if (key.empty()) {
        auto name = gdk_keyval_name(keyval);
        if (name) {
            key = name;
        }
        else {
            key = "<unknown>";
        }
    }
    container->append(*create_key(key.c_str()));

    return container;
}


struct ShortcutEdit : Gtk::Overlay {
    ShortcutEdit(XML::Node* node, ReadWrite::IO* io) : _node(node), _io(io) {
        set_name("ShortcutEdit");
        add_css_class("shortcut");
        set_child(_edit);
        _action_id = to_path(node).substr(strlen("/shortcuts/"));
        _edit.set_editable(false);
        // _edit.set_alignment(Gtk::Align::CENTER);
        auto pos = Gtk::Entry::IconPosition::SECONDARY;
        _edit.set_icon_from_icon_name("edit", pos);
        _edit.set_icon_activatable(true, pos);
        _edit.signal_icon_release().connect([this, node, io](auto icon){
            if (_in_edit_mode) {
                cancel_editing();
            }
            else {
                edit_shortcut();
            }
        });
        _edit.set_can_focus(false);
        _edit.set_focus_on_click(false);
        _edit.set_focusable(false);
        auto size = to_size(element_attr(node, "size"), WHOLE);
        _edit.set_size_request(size);
        _focus->signal_leave().connect([this]{ cancel_editing(); });
        containerize(*this);
        _confirm.set_size_request(size);
        _confirm.set_child(_content);
        _confirm.set_has_arrow(false);
        _confirm.set_autohide();
        _confirm.set_parent(*this);
        _confirm.signal_closed().connect([this]{ cancel_editing(); });
        _content.set_margin(4);
        _content.set_hexpand();
        _content.set_row_spacing(4);
        _content.set_column_spacing(4);
        _message.set_max_width_chars(40);
        _message.set_wrap();
        _message.set_wrap_mode(Pango::WrapMode::WORD);
        _content.attach(_message, 0, 0);
        auto hbox = Gtk::make_managed<Gtk::Box>();
        hbox->set_halign(Gtk::Align::CENTER);
        hbox->set_hexpand();
        hbox->set_spacing(4);
        hbox->append(_ok);
        hbox->append(_cancel);
        _ok.set_size_request(QUARTER);
        _cancel.set_size_request(QUARTER);
        _content.attach(*hbox, 0, 1);
        _cancel.signal_clicked().connect([this]{
            _confirm.popdown();
            edit_shortcut();
        });
        _ok.signal_clicked().connect([this]{
            end_shortcut_edit(_new_shortcut);
        });

        //todo: validate(shortcut, node, io);
        // auto keys = io->read(to_path(node));

        show_shortcuts(_action_id);
        auto keyctrl = Gtk::EventControllerKey::create();
        keyctrl->set_propagation_phase(Gtk::PropagationPhase::CAPTURE);

        // allow Esc to cancel out of shortcut editing
        keyctrl->signal_key_pressed().connect([this](auto keyval, auto keycode, auto mod){
            if (keyval == GDK_KEY_Escape && mod == Gdk::ModifierType::NO_MODIFIER_MASK) {
                cancel_editing();
            }
            if (_in_edit_mode && !Util::is_key_modifier(keyval)) {
                // assign new shortcut
                auto& shortcuts = Inkscape::Shortcuts::getInstance();
                auto state = static_cast<GdkModifierType>(mod);
                auto new_shortcut_key = shortcuts.get_from(nullptr, keyval, keycode, state, true);
// printf("new keyval: %x  code %d  mod: %x - '%s', kv: %x\n", keyval, keycode, mod, Util::get_accel_key_abbrev( new_shortcut_key).c_str(),new_shortcut_key.get_key());
                if (verify_shortcut(new_shortcut_key)) {
                    end_shortcut_edit(new_shortcut_key);
                }
            }
            return true;
        }, false);

        add_controller(keyctrl);
        add_controller(_focus);
    }

    ~ShortcutEdit() override = default;

    void edit_shortcut() {
        if (_in_edit_mode) return;

        _in_edit_mode = true;
        if (_keys) _keys->set_visible(false);
        _edit.set_icon_from_icon_name("close-button", Gtk::Entry::IconPosition::SECONDARY);
        _edit.set_can_focus();
        _edit.set_focusable();
        show_new_accel();
    }

    void cancel_editing() {
        if (_in_edit_mode) {
            _confirm.popdown();
            end_shortcut_edit({});
        }
    }

    void end_shortcut_edit(std::optional<Gtk::AccelKey> new_key) {
        if (!_in_edit_mode) return;

        _in_edit_mode = false;
        _confirm.popdown();
        _edit.set_icon_from_icon_name("edit", Gtk::Entry::IconPosition::SECONDARY);
        _edit.set_can_focus(false);
        _edit.set_focusable(false);
        // "unfocus"
        if (auto root = get_root()) {
            root->unset_focus();
        }
        if (new_key.has_value()) {
            // save
            auto& shortcuts = Inkscape::Shortcuts::getInstance();
            shortcuts.add_user_shortcut(_action_id, new_key.value());
            _signal_changed.emit(*new_key);
        }
        show_shortcuts(_action_id);
    }

    void show_new_accel() {
        _edit.set_placeholder_text(_("New accelerator..."));
        _edit.grab_focus();
        // _edit.grab_focus_without_selecting();
        if (_keys) _keys->set_visible(false);
    }

    void show_shortcuts(const std::string& action_id) {
        Util::ActionAccel accel(action_id);
        show_shortcuts(accel.getKeys());
    }

    void show_shortcuts(const std::vector<Gtk::AccelKey>& keys) {
        auto container = Gtk::make_managed<Gtk::Box>();
        container->set_spacing(1);
        container->set_valign(Gtk::Align::CENTER);
        container->set_halign(Gtk::Align::CENTER);
        bool first = true;
        auto display = Gdk::Display::get_default();
        for (auto key : keys) {
            if (!first) {
                container->append(*Gtk::make_managed<Gtk::Label>(",  "));
            }
            create_shortcut_label(display, container, key.get_key(), key.get_mod());
            first = false;
        }
        container->set_margin_end(16); // space for icon

        _edit.set_placeholder_text({});
        if (_keys) remove_overlay(*_keys);
        _keys = container;
        add_overlay(*_keys);
    }

    bool verify_shortcut(const Gtk::AccelKey& new_key) {
        _new_shortcut.reset();

        if (new_key.is_null()) return false;

        // test roundtrip; gtk cannot parse what gdk created... (like shift+option+1)
        auto test = Util::parse_accelerator_string(Util::get_accel_key_abbrev(new_key));
        if (Util::get_accel_key_abbrev(test).empty()) return false;

        Util::ActionAccel action_accel(_action_id);
        for (auto& acc_key : action_accel.getKeys()) {
            // same accelerator?
            if (new_key.get_key() == acc_key.get_key() && new_key.get_mod() == acc_key.get_mod()) {
                return true;
            }
        }

        auto iapp = InkscapeApplication::instance();
        InkActionExtraData& action_data = iapp->get_action_extra_data();

        // Check if there is currently an action assigned to this shortcut; if yes ask if the shortcut should be reassigned
        auto& shortcuts = Inkscape::Shortcuts::getInstance();
        Glib::ustring action_name;
        Glib::ustring accel = Gtk::Accelerator::name(new_key.get_key(), new_key.get_mod());
        const auto& actions = shortcuts.get_actions(accel);

        for (auto possible_action : actions) {
            if (action_data.isSameContext(_action_id, possible_action)) {
                // TODO: Reformat the data attached here so it's compatible with action_data
                action_name = possible_action;
                break;
            }
        }

        _new_shortcut = new_key;
        show_shortcuts({new_key});

        if (!action_name.empty()) {
            auto action_label = action_data.get_label_for_action(action_name);
            _message.set_markup(Glib::ustring::compose(_("This shortcut is already assigned to <b>%1</b>."),  Glib::Markup::escape_text(action_label.empty() ? action_name : action_label)));
            Inkscape::UI::popup_at(_confirm, *this, get_width() / 2, get_height() + 1);

            // wait for user's confirmation
            return false;
        }
        else {
            // shortcuts.add_user_shortcut(_action_id, new_key);

            return true; // done
        }
    }

    sigc::signal<void (const Gtk::AccelKey&)> _signal_changed;
    Gtk::Popover _confirm;
    Gtk::Widget* _keys = nullptr;
    Gtk::Entry _edit;
    Gtk::Grid _content;
    Gtk::Label _message;
    Gtk::Button _ok{_("Reassign")};
    Gtk::Button _cancel{_("Cancel")};
    ReadWrite::IO* _io;
    XML::Node* _node;
    bool _in_edit_mode = false;
    std::string _action_id;
    Glib::RefPtr<Gtk::EventControllerFocus> _focus = Gtk::EventControllerFocus::create();
    std::optional<Gtk::AccelKey> _new_shortcut;
};


Gtk::Widget* create_ui_element(Context& ctx, XML::Node* node);
void build_ui(Context& ctx, Gtk::Widget* parent, XML::Node* node, std::function<void (Gtk::Widget*)> append = {});

// Header of the expandable panel; host for icon, label, and more
struct Header : Gtk::Box {
    Header(const Glib::ustring& title, std::string_view icon_name) {
        add_css_class("header");
        set_hexpand();
        _header = Gtk::make_managed<Gtk::Button>();
        _header->set_has_frame(false);
        _header->set_hexpand();
        _header->set_focus_on_click(false);
        _header->add_css_class("button");
        auto box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL);
        if (auto icon = create_icon(icon_name)) {
            box->append(*icon);
        }
        // title
        auto l = Gtk::make_managed<Gtk::Label>(title);
        box->append(*l);
        // shortcut
        _shortcut = Gtk::make_managed<Gtk::Label>();
        _shortcut->add_css_class("panel-shortcut");
        _shortcut->set_xalign(0);
        _shortcut->set_hexpand();
        box->append(*_shortcut);
        // "expander" icon
        _arrow = create_icon("pan-down");
        box->append(*_arrow);
        _header->set_child(*box);
        append(*_header);
    }

    Gtk::Button* button() {
        return _header;
    }

    void set_icon(const Glib::ustring& icon) {
        _arrow->set_from_icon_name(icon);
    }

    Gtk::Image* _arrow;
    Gtk::Button* _header;
    Gtk::Label* _shortcut;
};

// Collapsible panel with a header
struct Panel : Gtk::Box {
    Panel(bool indent) : Box(Gtk::Orientation::VERTICAL) {
        add_css_class("panel");
        // content of the panel goes into the collapsible subgroup
        _subgroup = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL);
        _subgroup->add_css_class("group");
        if (indent) {
            _subgroup->add_css_class("indent");
        }
        append(*_subgroup);
    }
    ~Panel() override = default;

    bool is_expanded() const {
        return _subgroup->get_visible();
    }

    // std::string get_panel_unique_id() const {
    //     if (auto node = get_widget_node(this)) {
    //         auto section = element_attr(node->parent(), "id");
    //         auto id = element_attr(node, "id");
    //         return std::string(section) + "_" + std::string(id);
    //     }
    //     return {};
    // }

    void set_expanded(bool expand = true) {
        _subgroup->set_visible(expand);
        if (_header) _header->set_icon(expand ? "pan-down" : "pan-end");
        if (expand) {
            add_css_class("open");
        }
        else {
            remove_css_class("open");
        }
        if (expand) {
            // remember which panel is visible/expanded
            if (auto node = get_widget_node(this)) {
                // remember for each section separately
                auto section = to_string(element_attr(node->parent(), "id"));
                Preferences::get()->setString(settings_path + "showPanel/" + section, get_widget_id(this));
            }
        }
    }

    void add_header(Header* header) {
        if (_header) {
            std::cerr << "Panel already has header element set\n";
            return;
        }

        _header = header;
        prepend(*_header);
        // expand/collapse panel's content
        _header->button()->signal_clicked().connect([this]{
            set_expanded(!_subgroup->get_visible());
        });
    }

    Header* get_header() {
        return _header;
    }

    Gtk::Box* _subgroup;
    Header* _header = nullptr;
};

struct Section : Gtk::ListBoxRow {
    template <typename F>
    Panel* for_each_panel(F f) {
        auto& content = get_content(false);
        for (auto widget : content.get_children()) {
            if (auto panel = dynamic_cast<Panel*>(widget)) {
                if (f(panel)) return panel;
            }
        }
        return nullptr;
    }

    Section(Context& ctx, XML::Node* node): _root(node) {
        _box.add_css_class("section");
        _box.set_hexpand();
        if (!node) return;

        auto& content = get_content(false);
        build_ui(ctx, &content, node);

        for_each_panel([this](auto panel) {
            panel->set_expanded(false);
            if (auto header = panel->get_header()) {
                header->button()->signal_clicked().connect([this, panel] {
                    if (panel->is_expanded()) {
                        // collapse other panels, show only this one
                        expand_panel(panel);
                    }
                });
            }
            return false; // not done
        });

        // for (auto widget : content.get_children()) {
        //     if (auto panel = dynamic_cast<Panel*>(widget)) {
        //         panel->set_expanded(false);
        //         // panel_index++;
        //         if (auto header = panel->get_header()) {
        //             header->button()->signal_clicked().connect([this, panel] {
        //                 if (panel->is_expanded()) {
        //                     // collapse other panels, show only this one
        //                     expand_panel(panel);
        //                 }
        //             });
        //         }
        //     }
        // }
        auto title = to_label(node);
        auto label = Gtk::make_managed<Gtk::Label>(title);
        label->set_xalign(0);
        label->set_margin_start(4);
        set_child(*label);
    }

    // expand given panel, collapse all others
    void expand_panel(Panel* expand_panel) {
        for_each_panel([expand_panel](auto panel) {
            panel->set_expanded(panel == expand_panel);
            return false; // not finished
        });
        // auto& content = get_content(false);
        // for (auto widget : content.get_children()) {
        //     if (auto panel = dynamic_cast<Panel*>(widget)) {
        //         panel->set_expanded(panel == expand_panel);
        //     }
        // }
    }

    Panel* find_panel_by_id(const std::string& id) {
        return for_each_panel([&id](auto panel) {
            return get_widget_id(panel) == id;
        });
        // auto& content = get_content(false);
        // for (auto widget : content.get_children()) {
        //     if (auto panel = dynamic_cast<Panel*>(widget)) {
        //         if (get_widget_id(panel) == id) {
        //             return panel;
        //         }
        //     }
        // }
        // return nullptr;
    }

    Panel* get_first_panel() {
        return for_each_panel([](auto panel) {
            return true; // return first one found
        });

    }
    Gtk::Widget& get_content(bool create) {
        //TODO: create on first use?
        return _box;
    }
    void set_content(Gtk::Widget& content) {
        _box.append(content);
    }
private:
    Gtk::Box _box{Gtk::Orientation::VERTICAL, 0};
    XML::Node* _root;
};


void build_ui(Context& ctx, Gtk::Widget* parent, XML::Node* node, std::function<void (Gtk::Widget*)> append) {
    Gtk::Widget* previous = nullptr;

    for (auto element = node->firstChild(); element; element = element->next()) {
        auto name = element_name(element);
        if (name == "insert") {
            // insert template
            auto templ_name = element_attr(element, "template");
            if (auto it = ctx.templates.find(templ_name); it != ctx.templates.end()) {
                // clone template content, so child - parent relation works
                auto clone = it->second->duplicate(&ctx.ui);
                // pass parameters to cloned template from <insert> element
                subst_arguments(element, clone);
                element->appendChild(clone);
                build_ui(ctx, parent, clone, append);
            }
            else {
                //fmt lib?
                throw std::runtime_error(std::string("Missing template in settings UI: ") + std::string(templ_name));
            }
        }
        else {
            // parse nodes and create corresponding widget
            auto widget = create_ui_element(ctx, element);
            if (!widget) {
                // current node does not represent a widget; parse all other types here
                int gap = parse_element(element);
                if (gap > 0 && previous && !dynamic_cast<Header*>(previous)) {
                    // using margin here, because it is cheaper
                    previous->set_margin_bottom(gap);
                }
                else {
                    // no previous widget, so we need to inject widget-gap
                    widget = Gtk::make_managed<Gtk::Box>();
                    widget->set_size_request(1, gap);
                }
            }
            if (widget) {
                // remember the node in a widget, so we can query it later
                widget->set_data(NODE_KEY, element);
                if (append) {
                    append(widget);
                }
                else {
                    widget->insert_at_end(*parent);
                }
                // add observers for elements that need to be visible conditionally only
                if (!element_attr(element, "visible").empty()) {
                    add_visibility_observer(ctx, widget, element);
                }
            }

            previous = widget;
        }
    }

    // link radio buttons into single group, if any
    connect_radio_buttons(parent);
}

// find tooltip on given element or its group/row parents
Glib::ustring get_tooltip(XML::Node* node) {
    auto tooltip = to_string(element_attr(node, "tooltip"));
    while (tooltip.empty()) {
        node = node->parent();
        if (!node) break;
        auto name = element_name(node);
        if (name == "row" || name == "group" || name == "insert" || name == "template") {
            tooltip = to_string(element_attr(node, "tooltip"));
        }
        else {
            break;
        }
    }
    return tooltip;
}

// given XML node, create corresponding UI widget
Gtk::Widget* create_ui_element(Context& ctx, XML::Node* node) {
    auto name = element_name(node);
    auto label = to_label(node);
    auto tooltip = get_tooltip(node);
    auto path = to_string(element_attr(node, "path"));
    auto io = ctx.io;

    if (name == "panel") {
        auto indent = element_attr(node, "indent");
        // auto switch_path = std::string(element_attr(node, "switch"));
        auto panel = Gtk::make_managed<Panel>(indent.empty() || indent == "true");
        build_ui(ctx, panel->_subgroup, node, [panel](Gtk::Widget* widget) {
            if (auto header = dynamic_cast<Header*>(widget)) {
                panel->add_header(header);
            }
            else {
                panel->_subgroup->append(*widget);
            }
        });
        auto show_shortcut = [panel, io, node]{
            if (auto header = panel->get_header()) {
                set_shortcut(io, find_shortcut(node), header->_shortcut);
            }
        };
        if (auto shortcut = dynamic_cast<ShortcutEdit*>(find_widget_by_name(*panel->_subgroup, "ShortcutEdit", false))) {
            shortcut->_signal_changed.connect([=](auto&){
                show_shortcut();
            });
        }
        show_shortcut();
        return panel;
    }
    else if (name == "group") {
        auto type = element_attr(node, "type");
        auto group = Gtk::make_managed<Gtk::Box>(type == "radio" || type == "segmented" ? Gtk::Orientation::HORIZONTAL : Gtk::Orientation::VERTICAL);
        group->add_css_class("group");
        if (type.size()) {
            group->add_css_class(to_string(type));
        }
        auto subgroup = group;
        if (type == "radio" || type == "segmented") {
            group->add_css_class("linked");
        }
        build_ui(ctx, subgroup, node);
        return group;
    }
    else if (name == "row") {
        auto row = Gtk::make_managed<Gtk::Grid>();
        row->set_column_spacing(4);
        row->set_row_spacing(0);
        row->add_css_class("row");
        if (element_attr(node, "label").data()) {
            auto l = Gtk::make_managed<Gtk::Label>(label);
            l->add_css_class("label");
            l->set_xalign(0);
            l->set_valign(Gtk::Align::BASELINE);
            l->set_tooltip_text(tooltip);
            ctx.first_col->add_widget(*l);
            row->attach(*l, 0, 0);
        }
        if (element_attr(node, "reset-icon") == "yes") {
            auto icon = create_icon("reset");
            icon->set_tooltip_text(_("Requires restart to take effect"));
            icon->add_css_class("reset-icon");
            row->attach(*icon, 2, 0);
        }
        int new_row = 0;
        build_ui(ctx, row, node, [row, &new_row](Gtk::Widget* widget){ row->attach(*widget, 1, new_row++); });
        return row;
    }
    else if (name == "toggle") {
        auto mnemonic = false; // todo
        auto toggle = Gtk::make_managed<Gtk::ToggleButton>(label, mnemonic);
        // ellipsize long labels, so they obey grid column constraints
        if (auto l = dynamic_cast<Gtk::Label*>(toggle->get_children().at(0))) {
            l->set_ellipsize(Pango::EllipsizeMode::END);
            l->set_max_width_chars(0);
        }
        toggle->add_css_class("toggle");
        auto size = to_size(element_attr(node, "size"), THIRD);
        toggle->set_size_request(size);
        toggle->set_tooltip_text(tooltip);
        validate(toggle, node, io);
        set_widget(toggle, node, io);
        toggle->signal_toggled().connect([toggle, node, io] {
            bool on = toggle->get_active();
            auto value = element_attr(node, "value");
            if (!on) {
                // radio buttons only perform write when they are checked
                if (element_attr(node->parent(), "type") == "radio") return;
                // normal toggle "unchecked"
                value = "0";
            }
            io->write(to_path(node), value);
        });
        return toggle;
    }
    else if (name == "checkbox") {
        auto checkbox = Gtk::make_managed<Gtk::CheckButton>(label);
        checkbox->add_css_class("checkbox");
        checkbox->set_tooltip_text(tooltip);
        checkbox->set_active(read_bool(node, io));
        checkbox->set_halign(Gtk::Align::START);
        validate(checkbox, node, io);
        set_widget(checkbox, node, io);
        checkbox->signal_toggled().connect([checkbox, node, io]() {
            auto on = checkbox->get_active();
            auto onvalue = element_attr(node, "value");
            auto offvalue = element_attr(node, "off-value");
            io->write(to_path(node), on ? (onvalue.empty() ? "1" : onvalue) : (offvalue.empty() ? "0" : offvalue));
        });
        return checkbox;
    }
    else if (name == "radiobutton") {
        auto radiobutton = Gtk::make_managed<Gtk::CheckButton>(label);
        radiobutton->add_css_class("radio-button");
        radiobutton->set_tooltip_text(tooltip);
        radiobutton->set_halign(Gtk::Align::START);
        validate(radiobutton, node, io);
        set_widget(radiobutton, node, io);
        return radiobutton;
    }
    else if (name == "text") {
        //todo - text attributes
        auto content = node->firstChild();
        auto str = content ? content->content() : nullptr;
        auto text = Gtk::make_managed<Gtk::Label>(str ? str : "");
        text->add_css_class("text");
        text->set_valign(Gtk::Align::BASELINE_CENTER);
        auto cls = element_attr(node, "class");
        if (!cls.empty()) {
            text->add_css_class(to_string(cls));
        }
        return text;
    }
    else if (name == "number") {
        auto number = Gtk::make_managed<Widget::InkSpinButton>();
        number->add_css_class("number");
        auto min = element_attr(node, "min");
        auto max = element_attr(node, "max");
        if (min.empty() || max.empty()) {
            std::cerr << "Missing min/max range for <number> element in UI definition\n";
        }
        number->set_range(to_number(min), to_number(max));
        if (auto step = element_attr(node, "step"); step.size()) {
            number->set_step(to_number(step));
        }
        if (auto digits = element_attr(node, "precision"); digits.size()) {
            number->set_digits(to_number(digits));
        }
        if (auto unit = element_attr(node, "unit"); unit.size()) {
            number->set_suffix(to_string(unit));
        }
        if (auto scale = element_attr(node, "scaling-factor"); scale.size()) {
            number->set_scaling_factor(to_number(scale));
        }
        auto size = to_size(element_attr(node, "size"), HALF);
        number->set_size_request(size);
        number->set_tooltip_text(tooltip);
        validate(number, node, io);
        set_widget(number, node, io);
        number->signal_value_changed().connect([node, io](auto value) {
            static constexpr auto digits10 = std::numeric_limits<double>::digits10;
            io->write(to_path(node), Inkscape::ustring::format_classic(std::setprecision(digits10), value).raw());
        });
        return number;
    }
    else if (name == "shortcut") {
        auto shortcut = Gtk::make_managed<ShortcutEdit>(node, ctx.io);
        return shortcut;
    }
    else if (name == "expander") {
        // auto mnemonic = false; // todo
        auto button = Gtk::make_managed<Gtk::Button>();
        auto box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL);
        box->append(*Gtk::make_managed<Gtk::Label>(label));
        auto icon = create_icon("pan-end");
        icon->set_margin_start(8);
        box->append(*icon);
        button->set_child(*box);
        button->add_css_class("expander");
        button->set_halign(Gtk::Align::START);
        button->set_has_frame(false);
        button->set_focus_on_click(false);
        auto panel = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL);
        auto group = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL);
        panel->append(*button);
        panel->append(*group);
        button->signal_clicked().connect([group, icon]() {
            // toggle expander
            group->set_visible(!group->get_visible());
            icon->set_from_icon_name(group->get_visible() ? "pan-down" : "pan-end");
        });
        build_ui(ctx, group, node);
        group->set_visible(false);
        button->set_tooltip_text(tooltip);
        return panel;
    }
    else if (name == "color-picker") {
        Colors::Color c{0xff000000, false};
        //todo: more than a color picker is needed here; style picker?
        auto picker = Gtk::make_managed<Widget::ColorPicker>(label, tooltip, c, false, false);
        picker->add_css_class("color-picker");
        picker->set_size_request(HALF);
        validate(picker, node, io);
        return picker;
    }
    else if (name == "button") {
        auto button = Gtk::make_managed<Gtk::Button>(label);
        button->set_tooltip_text(tooltip);
        button->set_size_request(HALF);
        auto icon = element_attr(node, "icon");
        if (icon.size()) {
            auto box = Gtk::make_managed<Gtk::Box>();
            box->set_spacing(4);
            box->append(*create_icon(icon));
            box->append(*Gtk::make_managed<Gtk::Label>(label));
            box->set_halign(Gtk::Align::CENTER);
            button->set_child(*box);
        }
        //todo rest - action to fire
        return button;
    }
    else if (name == "header") {
        auto icon = element_attr(node, "icon");
        auto header = Gtk::make_managed<Header>(label, icon);
        build_ui(ctx, header, node);
        return header;
    }
    else if (name == "switch") {
        auto toggle_switch = Gtk::make_managed<Gtk::Switch>();
        toggle_switch->add_css_class("switch");
        toggle_switch->add_css_class("small");
        toggle_switch->set_tooltip_text(tooltip);
        toggle_switch->set_valign(Gtk::Align::CENTER);
        validate(toggle_switch, node, io);
        set_widget(toggle_switch, node, io);
        // connect on/off
        toggle_switch->property_state().signal_changed().connect([toggle_switch, io, node] {
            io->write(to_path(node), toggle_switch->get_state() ? "1" : "0");
        });
        return toggle_switch;
    }
    else if (name == "path") {
        //TODO: path editor
        auto wnd = Gtk::make_managed<Gtk::ScrolledWindow>();
        wnd->set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);
        auto path_edit = Gtk::make_managed<Gtk::TextView>();
        path_edit->set_wrap_mode(Gtk::WrapMode::WORD);
        path_edit->set_pixels_above_lines(0);
        path_edit->set_pixels_below_lines(2);
        path_edit->set_left_margin(3);
        path_edit->set_right_margin(3);
        path_edit->set_top_margin(3);
        validate(wnd, node, io);
        set_widget(path_edit, node, io, '|');
        wnd->set_size_request(WHOLE, 120);
        wnd->set_child(*path_edit);
        wnd->set_has_frame();
        path_edit->get_buffer()->signal_changed().connect([path_edit, io, node]{
            std::string value = path_edit->get_buffer()->get_text().raw();
            std::replace(value.begin(), value.end(), '\n', '|');
            io->write(to_path(node), value);
        });
        return wnd;
    }
    else if (name == "selector") {
        auto search = element_attr(node, "search") == "yes";
        auto selector = Settings::create_combobox(element_attr(node, "source"), ctx.scaling_factor, search);
        if (!selector) {
            throw std::runtime_error(std::string("Selector's source is not known: ") + std::string(element_attr(node, "source")));
        }
        selector->add_css_class("selector");
        auto size = to_size(element_attr(node, "size"), WHOLE);
        selector->set_size_request(size);
        return selector;
    }
    else if (name == "font-selector") {
        auto font_selector = Gtk::make_managed<Gtk::Entry>();
        font_selector->add_css_class("font-selector");
        auto pos = Gtk::Entry::IconPosition::SECONDARY;
        font_selector->set_icon_from_icon_name("edit", pos);
        font_selector->set_icon_activatable(true, pos);
        font_selector->signal_icon_release().connect([node, io](auto icon){
            auto dlg = std::make_unique<Gtk::FontChooserDialog>();
            // if (_in_edit_mode) {
            //     cancel_editing();
            // }
            // else {
            //     edit_shortcut();
            // }
        });
        font_selector->set_can_focus(false);
        font_selector->set_focus_on_click(false);
        font_selector->set_focusable(false);
        auto size = to_size(element_attr(node, "size"), WHOLE);
        font_selector->set_size_request(size);

        return font_selector;
    }
    else if (name == "ruler") {
        auto ruler = Gtk::make_managed<Widget::ZoomCorrRuler>();
        ruler->set_hexpand();
        ruler->add_css_class("ruler");
        return ruler;
    }
    else {
        // all other elements are not handled here
        return nullptr;
    }
}

Section* create_section(Context& ctx, XML::Node* node, Gtk::Widget* page, const Glib::ustring& page_title) {
    auto section = Gtk::make_managed<Section>(ctx, node);
    auto title = element_attr(node, "label");
    auto label = Gtk::make_managed<Gtk::Label>(Glib::ustring(title.data(), title.size()));
    if (page) {
        label->set_text(page_title);
        section->set_content(*page);
    }
    label->set_xalign(0);
    label->set_margin_start(4);
    // label->set_margin(5);
    section->set_child(*label);
    section->set_visible();
    section->set_data(NODE_KEY, node);
    return section;
}

} // namespace

void SettingsDialog::collect_templates(XML::Node* node, Templates& templates) {
    for (auto element = node->firstChild(); element; element = element->next()) {
        if (element_name(element) == "template") {
            if (auto name = element_attr(element, "name"); !name.empty()) {
                templates[std::string(name)] = element;
            }
            else {
                std::cerr << "Missing template name in UI settings\n";
            }
        }
        else {
            std::cerr << "Expected element 'template' in UI settings\n";
        }
    }
}

SettingsDialog::SettingsDialog(Gtk::Window& parent):
    Dialog(_("Inkscape Settings"), true),
    _builder(create_builder("settings-dialog.ui")),
    _content(get_widget<Gtk::Box>(_builder, "page-content")),
    _search(get_widget<Gtk::SearchEntry2>(_builder, "global-search")),
    _pages(get_widget<Gtk::ListBox>(_builder, "page-list")),
    _ui(get_ui_xml())
{
    set_default_size(800, 600);
    set_name("Settings");

    auto objects = _builder->get_objects();

    for (const auto& obj : objects) {
        auto spin = std::dynamic_pointer_cast<UI::Widget::PreferenceSpinButton>(obj);
        if (!spin) continue;

        auto unit_id = spin->property_unit_menu().get_value();
        if (unit_id.empty()) continue;

        auto& unit_widget = get_widget<UI::Widget::PreferenceUnitMenu>(_builder, unit_id.c_str());
        unit_widget.get_unit_menu().resetUnitType(Inkscape::Util::UNIT_TYPE_LINEAR);
        spin->bind_unit_menu(unit_widget.get_unit_menu());
    }

    {
        // Rotation snaps are stored as "snaps per pi" but UI shows degrees.
        // degrees = 180 / snaps_per_pi
        auto& spin = get_widget<UI::Widget::PreferenceSpinButton>(_builder, "rotation-snaps-deg");
        spin.set_transformers(
            [] (double degrees) {
                return degrees == 0.0 ? 0.0 : 180.0 / degrees;
            },
            [] (double snaps_per_pi) {
                return snaps_per_pi == 0.0 ? 0.0 : 180.0 / snaps_per_pi;
            }
        );
    }

    // _page_selector.append(_search);
    // _search.set_max_width_chars(6);
    _search.set_placeholder_text(_("Search"));
    _search.signal_search_changed().connect([this]() {
        // filter
    });
    // _page_selector.append(_pages);
    // _page_selector.set_name("PageSelector");
    // _pages.set_vexpand();
    // _pages.set_name("Pages");
    // _hbox.append(_page_selector);
    // _hbox.append(*Gtk::make_managed<Gtk::Separator>(Gtk::Orientation::VERTICAL));
    // _hbox.append(_wnd);
    // _wnd.set_expand();
    // _wnd.set_has_frame(false);
    // _wnd.set_child(_content);
    // _page_selector.set_margin_end(8);
    // _content.set_margin_start(8);
    // _content.set_margin_end(8);
    // _content.set_expand();
_builder->get_widget<Gtk::Entry>("user-config-path")->set_text("/Users/mike/Library/Application Support/org.inkscape.Inkscape/config/inkscape");

    // access to preferences
    _io = std::make_unique<PreferencesIO>(get_root()->get_display());

    int pages = 0;
    auto ui = _ui->root();
    try {
        Context ctx(*_ui, _templates, _io.get(), *_observers, *_visibility, get_scale_factor());

        for (auto node = ui->firstChild(); node; node = node->next()) {
            auto name = element_name(node);
            if (name == "templates") {
                // ui templates for reuse
                collect_templates(node, _templates);
            }
            else if (name == "section") {
                // sections (or pages)
                ctx.first_col = Gtk::SizeGroup::create(Gtk::SizeGroup::Mode::HORIZONTAL);
                auto section = create_section(ctx, node, 0, "");
                _pages.append(*section);
                pages++;
                _content.append(section->get_content(false));
            }
            else if (name == "comment") {
            }
            else {
                // anything else?
                throw std::runtime_error(std::string("Unexpected element in settings UI: ") + std::string(name));
            }
        }
    }
    catch (std::exception& ex) {
        std::cerr << "Error creating settings dialog: " << ex.what() << std::endl;
        return;
    }

    Context ctx(*_ui, _templates, _io.get(), *_observers, *_visibility, get_scale_factor());
    for (auto obj : objects) {
        auto widget = dynamic_cast<Gtk::Grid*>(obj.get());
        if (widget && widget->get_name() == "Page") {
            auto title = dynamic_cast<Gtk::Label*>(widget->get_child_at(1, 0));
     // printf("page: '%s'\n", title->get_text().c_str());
            if (title) title->set_visible(false);
            auto page = create_section(ctx, nullptr, widget, title->get_text());
            // page->set_hexpand();
            _pages.append(*page);
            pages++;
        }
    }

    // auto test = create_section(ctx, 0, &get_widget<Gtk::Grid>(_builder, "ui-page"), "User Interface");
    // _pages.append(*test);
    // pages++;
    // _content.append(test->get_content(false));


    auto selected_page = Preferences::get()->getString(settings_path + "selectedPage");

    _pages.signal_row_selected().connect([this](Gtk::ListBoxRow* row) {
        // show content of selected page
        if (auto section = dynamic_cast<Section*>(row)) {
            for (auto c : _content.get_children()) {
                _content.remove(*c);
            }
            _content.append(section->get_content(true));

            if (auto element = get_widget_node(row)) {
                auto id = element_attr(element, "id");
                Preferences::get()->setString(settings_path + "selectedPage", std::string(id));
            }

            auto expanded = Preferences::get()->getString(settings_path + "showPanel/" + get_widget_id(section));
            auto panel = section->find_panel_by_id(expanded);
            if (!panel) {
                panel = section->get_first_panel();
            }
            if (panel) {
                section->expand_panel(panel);
            }
        }
    });

    for (int i = 0; i < pages; ++i) {
        if (auto row = _pages.get_row_at_index(i)) {
            auto element = get_widget_node(row);
            if (!element) continue;
            auto id = element_attr(element, "id");
            if (id == selected_page.raw()) {
                _pages.select_row(*row);
                break;
            }
        }
    }

    if (!_pages.get_selected_row()) {
        if (auto row = _pages.get_row_at_index(0)) {
            _pages.select_row(*row);
        }
    }

    get_content_area()->append(get_widget<Gtk::Box>(_builder, "main-box"));
    set_transient_for(parent);
    set_visible();
    _pages.grab_focus();

    auto& s = get_widget<UI::Widget::InkSpinButton>(_builder, "ui-text-scale");
    s.signal_value_changed().connect([](auto value) {
        // set text scale
        if (auto const settings = Gtk::Settings::get_default()) {
            auto normal = 72 * 1024;
            auto adjusted = static_cast<int>(value / 100 * normal);
            settings->property_gtk_xft_dpi().set_value(adjusted);
        }
    });
}

SettingsDialog::~SettingsDialog() {
    //todo: destroy widgets first

    Inkscape::GC::release(_ui);
}

} // namespaces
