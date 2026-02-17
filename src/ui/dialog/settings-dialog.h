// SPDX-License-Identifier: GPL-2.0-or-later
//
// Created by Michael Kowalski on 1/31/25.
//

#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include <gtkmm/dialog.h>
#include <gtkmm/listbox.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/searchentry2.h>

#include "preferences.h"


namespace Gtk {
class Builder;
}

namespace Inkscape::XML {
class Node;
struct Document;
}

namespace Inkscape::UI::Dialog {

namespace ReadWrite {
struct IO {
    virtual std::optional<std::string> read(const std::string& path) = 0;
    virtual void write(const std::string& path, std::string_view value) = 0;
    virtual bool is_valid(const std::string& path) = 0;
    virtual ~IO() = default;
};
} // ReadWrite

class SettingsDialog : public  Gtk::Dialog {
public:
    SettingsDialog(Gtk::Window& parent);
    ~SettingsDialog() override;

private:
    Glib::RefPtr<Gtk::Builder> _builder;
    // map of all UI templates (with heterogeneous lookup)
    using Templates = std::map<std::string, XML::Node*, std::less<>>;
    void collect_templates(XML::Node* node, Templates& templates);

    using Observers = std::map<std::string, std::unique_ptr<Preferences::PreferencesObserver>>;
    std::unique_ptr<Observers> _observers = std::make_unique<Observers>();

    std::unique_ptr<std::map<std::string, std::vector<Gtk::Widget*>>> _visibility = std::make_unique<std::map<std::string, std::vector<Gtk::Widget*>>>();

    std::unique_ptr<ReadWrite::IO> _io;
    // Gtk::Box _hbox{Gtk::Orientation::HORIZONTAL, 0};
    // Gtk::Box& _page_selector;//{Gtk::Orientation::VERTICAL, 8};
    Gtk::SearchEntry2& _search;
    Gtk::ListBox& _pages;
    // Gtk::ScrolledWindow _wnd;
    Gtk::Box& _content;//{Gtk::Orientation::VERTICAL, 4};
    XML::Document* _ui;
    Templates _templates;
};

} // namespace

#endif //SETTINGS_DIALOG_H
