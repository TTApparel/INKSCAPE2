// SPDX-License-Identifier: GPL-2.0-or-later
//
// Created by Michael Kowalski on 2/13/25.
//

#include "key-helpers.h"

#include <gdkmm/enums.h>
#include <gtkmm/accelkey.h>
#include <gtkmm/accelerator.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <regex>
#include <string>
#include <vector>

namespace Inkscape::Util {

Gtk::AccelKey transform_key_value(const Glib::RefPtr<Gdk::Display>& display, int keyval, Gdk::ModifierType mod) {
    if (!display) return Gtk::AccelKey(keyval, mod);

#ifdef __APPLE__
    // Special treatment for all key combinations with <option> modifier.
    // Option+key inserts symbols. We need to retrieve key letter in this case,
    // so we can presented 'A' in Option+A as 'A' and not an Angstrom symbol for instance.

    // was <option> modifier held down?
    if ((mod & Gdk::ModifierType::ALT_MASK) != Gdk::ModifierType::NO_MODIFIER_MASK) {
        int keycode = -1;
        int level = 0;

        GdkKeymapKey* map = nullptr;
        int len = 0;
        gdk_display_map_keyval(display->gobj(), keyval, &map, &len);
        if (len > 0) {
            keycode = map[0].keycode;
            level = map[0].level;
        }
// for (int i =0; i<len; ++i) {
//     printf("keyval: %x  grp %d  lvl %d  kcode: %d \n", keyval, map[i].group, map[i].level,  map[i].keycode);
// }
        g_free(map);


        guint* keys = nullptr;
        GdkKeymapKey* keymap = nullptr;
        int n = 0;
        gdk_display_map_keycode(display->gobj(), keycode, &keymap, &keys, &n);

// for (int i =0; i<n; ++i) {
//     printf("kval: %x  grp %d  lvl %d  kcode: %d \n", keys[i], keymap[i].group, keymap[i].level,  keymap[i].keycode);
// }
        if (n > 1) {
            // use normal keyval /*or with <shift>,*/ but without <option> - option produces symbols
            keyval = keys[0];//;(mod & Gdk::ModifierType::SHIFT_MASK) != Gdk::ModifierType::NO_MODIFIER_MASK ? 1 : 0];

            // <shift> is typically removed from shortcuts
            if ((mod & Gdk::ModifierType::SHIFT_MASK) == Gdk::ModifierType::NO_MODIFIER_MASK) {
                if (level == 1 && keymap[0].level == 0) {
                    // input 'keyval' was obtained with <shift> held down; since we have now transformed it
                    // to an "unshifted" value, we need to add <shift> to the modifiers
                    mod |= Gdk::ModifierType::SHIFT_MASK;
                }
            }
        }
        g_free(keys);
        g_free(keymap);
    }
#endif

    return Gtk::AccelKey(keyval, mod);
}

bool is_key_modifier(int keyval) {
    switch (keyval) {
    case GDK_KEY_Shift_L: return true;
    case GDK_KEY_Shift_R: return true;
    case GDK_KEY_Control_L: return true;
    case GDK_KEY_Control_R: return true;
    // case GDK_KEY_Caps_Lock: return true; 0xffe5
    // case GDK_KEY_Shift_Lock: return true; 0xffe6
    case GDK_KEY_Meta_L: return true;
    case GDK_KEY_Meta_R: return true;
    case GDK_KEY_Alt_L: return true;
    case GDK_KEY_Alt_R: return true;
    case GDK_KEY_Super_L: return true;
    case GDK_KEY_Super_R: return true;
    case GDK_KEY_Hyper_L: return true;
    case GDK_KEY_Hyper_R: return true;

    default: return false;
    }
}

Glib::ustring format_accel_keys(const Glib::RefPtr<Gdk::Display>& display, const std::vector<Gtk::AccelKey>& accels) {
    if (accels.empty()) return {};

    Glib::ustring output;
    output.reserve(64); // speculative

    for (auto& key : accels) {
        if (!output.empty()) output += ", ";

        auto accel = transform_key_value(display, key.get_key(), key.get_mod());
        output += Gtk::Accelerator::get_label(accel.get_key(), accel.get_mod());
    }

    return output;
}

Gtk::AccelKey parse_accelerator_string(const Glib::ustring& accelerator) {
    static const std::regex match_accel("(<.*?>)?(<.*?>)?(<.*?>)?(<.*?>)?(<.*?>)?(<.*?>)?U\\+([0-9A-F]+)");
    std::smatch match;
    if (std::regex_search(accelerator.raw(), match, match_accel) && match.size() > 1) {
        Gdk::ModifierType mod = Gdk::ModifierType::NO_MODIFIER_MASK;
        for (int i = 1; i < match.size() - 1; ++i) {
            if (!match[i].matched) continue;

            auto mod_key = match[i].str();
            if (mod_key == "<Alt>") {
                mod |= Gdk::ModifierType::ALT_MASK;
            }
            else if (mod_key == "<Shift>") {
                mod |= Gdk::ModifierType::SHIFT_MASK;
            }
            else if (mod_key == "<Control>") {
                mod |= Gdk::ModifierType::CONTROL_MASK;
            }
            else if (mod_key == "<Meta>") {
                mod |= Gdk::ModifierType::META_MASK;
            }
            else if (mod_key == "<Hyper>") {
                mod |= Gdk::ModifierType::HYPER_MASK;
            }
            else if (mod_key == "<Super>") {
                mod |= Gdk::ModifierType::SUPER_MASK;
            }
        }
        auto unicode = strtol(match[match.size() - 1].str().c_str(), nullptr, 16);
        // combine unicode with a keyval mask of 0x01000000 ("a directly encoded 24-bit UCS character"):
        int keyval = gdk_unicode_to_keyval(unicode);
        return Gtk::AccelKey(keyval, mod);
    }

    // delegate all other cases to gtk
    return Gtk::AccelKey(accelerator);
}

// On macOS we need to preserve Unicode keyvals exactly, so we can retrieve proper keys back;
// gdk makes them lowercase, which confuses decoding in some corner cases
Glib::ustring get_accel_key_abbrev(const Gtk::AccelKey& accel) {
#ifdef __APPLE__
    int unicode = gdk_keyval_to_unicode(accel.get_key());
    if (unicode < 0x80) {
        // ASCII or not valid accel; regular handler
        return accel.get_abbrev();
    }
    // High-ASCII and unicode: encode into U+xxxx form to preserve case as-is
    Glib::ustring str;
    str.reserve(64);
    auto mod = static_cast<GdkModifierType>(accel.get_mod());
    if (mod & GDK_SHIFT_MASK) {
        str.append("<Shift>");
    }
    if (mod & GDK_CONTROL_MASK) {
        str.append("<Control>");
    }
    if (mod & GDK_ALT_MASK) {
        str.append("<Alt>");
    }
    if (mod & GDK_META_MASK) {
        str.append("<Meta>");
    }
    if (mod & GDK_SUPER_MASK) {
        str.append("<Super>");
    }
    if (mod & GDK_HYPER_MASK) {
        str.append("<Hyper>");
    }

    str += "U+";
    char buf[16];
    snprintf(buf, 16, unicode > 0xffff ? "%06X" : "%04X", unicode);
    str += buf;

    return str;
#else
    return accel.get_abbrev();
#endif
}

} // namespace
