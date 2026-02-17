// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Authors:
 *   Martin Owens <doctormo@geek-2.com>
 *
 * Copyright (C) 2023-2025 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_COLORS_SPACES_BASE_H
#define SEEN_COLORS_SPACES_BASE_H

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "colors/parser.h"
#include "enum.h"
#include "colors/cms/profile.h"

constexpr double SCALE_UP(double v, double a, double b)
{
    return (v * (b - a)) + a;
}
constexpr double SCALE_DOWN(double v, double a, double b)
{
    return (v - a) / (b - a);
}

namespace Inkscape::Colors {
namespace CMS {
class Profile;
class TransformColor;
class GamutChecker;
} // namespace CMS
class Color;
class Manager;

namespace Space {

class Components;

class AnySpace : public std::enable_shared_from_this<AnySpace>
{
public:
    virtual ~AnySpace() = default;

    bool operator==(AnySpace const &other) const { return other.getName() == getName(); }
    bool operator!=(AnySpace const &other) const { return !(*this == other); };

    // Each space has a unique type enum for easier static use, use getComponentType
    // for seeing the internal type of the space, for example in the CMS space.
    bool operator==(Type type) const { return type == _type; }
    bool operator!=(Type type) const { return type != _type; }

    Type getType() const { return _type; }
    std::string const& getName() const { return _name; }
    std::string const& getShortName() const { return _shortName; }
    std::string getSvgName() const { return _svgNames.empty() ? "" : _svgNames[0]; }
    std::vector<std::string> const& getSvgNames() const { return _svgNames; }
    std::string const& getIcon() const { return _icon; }
    virtual Type getComponentType() const { return getType(); }
    virtual unsigned int getComponentCount() const { return _components; }
    virtual std::shared_ptr<Colors::CMS::Profile> const getProfile() const = 0;
    RenderingIntent getIntent() const { return _intent; }
    RenderingIntent getBestIntent(std::shared_ptr<AnySpace> const &to_space) const;
    // Specifies if this color space can be used for color interpolation in the render engine.
    virtual bool canInterpolateColors() const { return true; }
    // Some color spaces (like XYZ or LAB) do not put restrictions on valid ranges of values;
    // others (like sRGB) do, which means that channels outside those bounds represent colors out of gamut.
    bool isUnbounded() const { return _spaceIsUnbounded; }
    // Check if 'color' is out of gamut in '*this' color space;
    // use epsilon value to ignore some small deviations from valid domain (they can arise during conversions).
    bool isOutOfGamut(const Colors::Color& color, double eps = 0.0001);
    // Bring 'color' into gamut of '*this' color space
    Color toGamut(const Colors::Color& color);

    // isDirect is true when the child class implements getProfile
    // TODO: Turn this into a compile time flag, as we know at compile time if it's direct or not.
    virtual bool isDirect() const { return false; }

    Components const &getComponents(bool alpha = false) const;
    std::string const getPrefsPath() const { return "/colorselector/" + getName() + "/"; }

    virtual bool isValid() const { return true; }

    bool convert(std::vector<double> &io, std::shared_ptr<AnySpace> to_space) const;

protected:
    friend class Colors::Color;

    AnySpace(Type type, int components, std::string name, std::string shortName, std::string icon, bool spaceIsUnbounded = false);

    bool isValidData(std::vector<double> const &values) const;
    virtual std::vector<Parser> getParsers() const { return {}; }
    virtual std::string toString(std::vector<double> const &values, bool opacity = true) const = 0;

    bool profileToProfile(std::vector<double> &io, std::shared_ptr<AnySpace> to_space) const;
    virtual void spaceToProfile(std::vector<double> &io) const;
    virtual void profileToSpace(std::vector<double> &io) const;
    virtual bool overInk(std::vector<double> const &input) const { return false; }

    uint32_t toRGBA(std::vector<double> const &values, double opacity = 1.0) const;

    std::shared_ptr<Colors::CMS::Profile> srgb_profile = Colors::CMS::Profile::create_srgb();

    bool outOfGamut(std::vector<double> const &input, std::shared_ptr<AnySpace> to_space) const;

    RenderingIntent _intent = RenderingIntent::UNKNOWN;
    int _intent_priority = 0;
    std::vector<std::string> _svgNames;
private:
    mutable std::map<std::string, std::shared_ptr<Colors::CMS::TransformColor>> _transforms;
    mutable std::map<std::string, std::shared_ptr<Colors::CMS::GamutChecker>> _gamut_checkers;
    Type _type;
    int _components;
    std::string _name;
    std::string _shortName;
    std::string _icon;
    bool _spaceIsUnbounded;
};

} // namespace Space
} // namespace Inkscape::Colors

#endif // SEEN_COLORS_SPACES_BASE_H
