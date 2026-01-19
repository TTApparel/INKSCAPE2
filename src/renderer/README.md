# Inkscape SVG Renderer

This part of inkscape is designed to convert SVG data into rasters for use in either output to file, to screen or other processing where rasters are needed.

It is NOT a toolkit of generic Cairo utilities, or general tools for constructing rasters from other kinds of data. This means drawing Gtk widgets, shadows and other kinds of effects should not in any way be in this set of directories.

The renderer is laid out with a mot and bailey design, with inner rings and outter rings of functionality. Each layer is tested with units tests to ensure encapsulation. If your changes require expanding the number of objects required to build the unit test then you must think very carefully if the new objects truely are part of this inner ring and if you are breaking encapsulation.

Do not reference or import any headers from outside of this folder, unless they are simple utilities such as such parsers.

# Renderer::Surface

An image surface is a block of memory which can be written to by a Renderer::Context, or directly by a Renderer::PixelAccess (see below).

The image surface is statelsss, so it doesn't contain any information about the initial transformation, or other settings which would be used by the initalisation of the Context.

The images does have a size, a color space and a device scale.

# Renderer::Context

A drawing tool which takes a `Renderer::Surface` as it's input and is able to draw vector instructions onto that surface.

# Renderer::PixelAccess

A set of tools for editing the surface pixel by pixel with a threading pool. Often used for filter primitives, color space transformations and some other features.

The pixel access uses a Cairo Surface object directly so it can be seperable from Renderer::Surface at testing time, see `Renderer::PixelFilter` below.

# Renderer::PixelFilter::*

The deep down filter primitives which use a PixelAccess to apply well understood changes to a surface pixel by pixel.

In order to test the integration as well as the functionality, testing for each filter is split into two unit tests. Tests begining with `pixel-access-filter-` test the core functionality of the pixel modifications. All the various ways in which a filter can be flexed should be tested here. These tests use `pixel-access-testbase.h` as their testing utilties.

The second test begins with `surface-filter-` and has the name suffix as the pixel-access test. This is typically a copy of one of the pixel-access tests to ensure the Inkscape Surface can call the filter through the filter template. The interface for the filter function is particular and this test ensures it has been coded correctly. It uses `surface-testbase.h` as it's testing utility and filters are fed to `Renderer::Surfaces` instead of `Cairo::Surfaces`. Each filter function should result in one test.

Filters should have a `filter` const function which MAY return data, but must only take one, two or three `PixelAccess` templated arguments.

# Renderer::Drawing

A renderable drawing item.

# Renderer::DrawingFilter::*

Filters which reflect the SVG specification of available filters, they use PixelFilters as well as painting operations in a Context and remember where sources and destinations are for the whole filter stack.

