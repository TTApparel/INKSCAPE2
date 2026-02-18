# Repository Review: INKSCAPE2

## High-level purpose
This repository contains the Inkscape vector graphics editor source code, centered around an SVG-first document model and a desktop GUI application (`inkscape`) plus a viewer binary (`inkview`).

## Build system and project shape
- Uses CMake as the top-level build system.
- C++17 is required.
- The build creates:
  - `inkscape` executable
  - `inkview` executable
  - shared/base library `inkscape_base` that contains most core functionality.
- The root CMake configuration exposes many options for feature toggles (SVG2, OpenMP, Poppler, ImageMagick/GraphicsMagick, DBus, gspell, NLS, sanitizer/profiling knobs).

## Runtime architecture at a glance
- Main entrypoint in `src/inkscape-main.cpp`:
  - normalizes legacy CLI options,
  - configures environment variables for themes/extensions,
  - handles platform-specific environment setup (macOS app bundle, Windows console/DLL path behavior),
  - runs the `Gio::Application` through `InkscapeApplication::singleton()`.
- `InkscapeApplication` (in `src/inkscape-application.h`) is the application-level orchestrator:
  - owns/coordinates documents and windows,
  - tracks active document/selection/view/window,
  - handles startup/open/new/quit paths,
  - supports command-style action execution and headless/batch usage modes.

## Source tree organization
The `src/` tree is modularized by responsibility. Some notable areas:
- `src/actions` - command/action handlers.
- `src/object`, `src/svg`, `src/xml` - document/object model and SVG/XML handling.
- `src/ui`, `src/display`, `src/widgets` - GUI and desktop interaction layers.
- `src/live_effects` - Live Path Effects subsystem.
- `src/io`, `src/extension` - import/export and extension framework integration.
- `src/3rdparty` - vendored dependencies (e.g., 2geom, adaptagrams, depixelize, autotrace, libcroco).

## Data/assets and packaging
- `share/` holds install-time resources (icons, templates, filters, palettes, UI resources, tutorials, symbols, etc.).
- `packaging/` and `snap/` include packaging/distribution assets.
- `po/` contains localization infrastructure.

## Testing strategy
- Tests are wired through CTest/GTest in `testfiles/`.
- Includes C++ unit tests, CLI tests, rendering tests, and optional fuzz target support.
- Test setup includes an `inkscape_datadir` symlink mechanism so tests can run from build trees without install.

## Development and dependency notes
- Dependency/setup guidance is in `INSTALL.md` and references submodules and platform-specific docs.
- Extensions are intentionally externalized into a separate repository and consumed as a submodule.

## Summary assessment
This is a large, mature, cross-platform C++ desktop application with:
- clear build/configuration surface via CMake,
- a layered architecture (application orchestration -> document model/action system -> UI/rendering/extensions),
- substantial platform-specific startup/packaging considerations,
- and an established test harness spanning unit, CLI, and rendering validation.
