// SPDX-License-Identifier: GPL-2.0-or-later
//
// Created by Michael Kowalski on 10/24/25.
//

#include "preference-widgets.h"

#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <locale>
#include <sstream>

#include "ui/widget/unit-menu.h"

namespace Inkscape::UI::Widget {

GType PreferenceCheckButton::gtype = 0;

void PreferenceCheckButton::construct() {
    set_name("PreferenceCheckButton");

    auto set_value = [this]{
        auto path = prop_path.get_value();
        if (!path.empty()) {
            if (get_accessible_role() == Role::RADIO) {
                // radio button
                auto value = Preferences::get()->getInt(path);
                set_active(value == prop_enum.get_value());
            }
            else {
                // check box
                auto value = Preferences::get()->getBool(path);
                set_active(value);
            }
        }
    };

    auto write_value = [this]{
        if (_updating.pending()) return;

        auto path = prop_path.get_value();
        if (path.empty()) return;

        if (get_accessible_role() == Role::RADIO) {
            if (get_active()) {
                Preferences::get()->setInt(path, prop_enum.get_value());
            }
        }
        else {
            Preferences::get()->setBool(path, get_active());
        }
    };

    property_pref_path().signal_changed().connect([this, set_value] {
        auto guard = _updating.block();
        set_value();
    });

    signal_toggled().connect([write_value] { write_value(); });

    {
        auto guard = _updating.block();
        set_value();
    }
}

GType PreferenceSpinButton::gtype = 0;

void PreferenceSpinButton::write_unit_pref() {
    auto path = prop_path.get_value();
    if (path.empty() || !_unit_menu) return;

    auto unit = _unit_menu->getUnitAbbr();
    if (unit.empty()) {
        unit = _last_unit.empty() ? Glib::ustring("px") : _last_unit;
    }
    _last_unit = unit;

    std::ostringstream ss;
    ss.imbue(std::locale::classic());
    ss << std::fixed << std::setprecision(get_digits()) << get_value();
    Preferences::get()->setString(path, Glib::ustring(ss.str()) + unit);
}

void PreferenceSpinButton::load_unit_pref() {
    auto path = prop_path.get_value();
    if (path.empty()) return;

    auto prefs = Preferences::get();
    auto entry = prefs->getEntry(path);
    auto unit = entry.getUnit();
    auto str = entry.getString();

    double value = 0.0;
    if (!str.empty()) {
        value = strtod(str.c_str(), nullptr);
    }

    if (unit.empty()) {
        unit = _last_unit.empty() ? Glib::ustring("px") : _last_unit;
    }
    _last_unit = unit;

    if (_unit_menu) {
        _unit_menu->setUnit(unit);
    }
    set_value(value);
}

void PreferenceSpinButton::construct() {
    set_name("PreferenceSpinButton");

    auto set_num_value = [this]{
        auto path = prop_path.get_value();
        if (path.empty()) return;

        if (_unit_menu) {
            // Scalar+unit stored together in a single preference string (e.g. "2px")
            load_unit_pref();
        }
        else {
            // Plain numeric preference
            if (get_digits() == 0) {
                // no decimal digits - use integer
                auto value = Preferences::get()->getInt(path);
                set_value(value);
            }
            else {
                auto value = Preferences::get()->getDouble(path);
                set_value(value);
            }
        }
    };

    auto write_num_value = [this]{
        auto path = prop_path.get_value();
        if (path.empty() || _updating.pending()) return;

        if (_unit_menu) {
            write_unit_pref();
        }
        else {
            if (get_digits() == 0) {
                Preferences::get()->setInt(path, static_cast<int>(std::lround(get_value())));
            }
            else {
                Preferences::get()->setDouble(path, get_value());
            }
        }
    };

    property_pref_path().signal_changed().connect([this, set_num_value]{
        auto guard = _updating.block();
        set_num_value();
    });

    signal_value_changed().connect([this, write_num_value](double) {
        write_num_value();
    });

    {
        auto guard = _updating.block();
        set_num_value();
    }
}

void PreferenceSpinButton::bind_unit_menu(UnitMenu& menu) {
    _unit_menu = &menu;
    _unit_menu->resetUnitType(UNIT_TYPE_LINEAR);

    _unit_menu->signal_changed().connect([this] {
        if (_updating.pending()) return;

        auto guard = _updating.block();
        // keep current numeric value, only update preference string with the new unit
        write_unit_pref();
    });

    {
        auto guard = _updating.block();
        // Refresh value + unit from prefs now that menu exists.
        load_unit_pref();
    }
}

} // namespace
